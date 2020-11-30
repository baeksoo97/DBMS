#include "lock_manager.h"
#include "transaction_manager.h"

unordered_map <lock_key_t, lock_entry_t, hash_pair> lock_table;
pthread_mutex_t lock_manager_latch;

/*
 * Initialize any data structured required for implementing lock table
 */
int init_lock_table(void){
    int ret = pthread_mutex_init(&lock_manager_latch, NULL);
    if (ret != 0) return -1;
    else return 0;
}

void init_new_lock(int table_id, int64_t key, int trx_id, int lock_mode, lock_t * lock_obj){
    lock_obj->next = NULL;
    lock_obj->trx_prev_lock = NULL;
    lock_obj->trx_next_lock = NULL;
    lock_obj->sentinel = &lock_table[make_pair(table_id, key)];
    pthread_cond_init(&(lock_obj->cond), NULL);
    lock_obj->lock_mode = lock_mode;
    lock_obj->owner_trx_id = trx_id;

    // update is_waiting, prev should be done in caller
    // lock_obj->prev = NULL;
    // lock_obj->is_waiting = true;
}

bool check_deadlock(int trx_id, lock_t * lock_obj){
    set<int> visit_set;
    set<int>::iterator it;

    visit_set = get_wait_for_graph(lock_obj, visit_set);

    for(it = visit_set.begin(); it != visit_set.end(); it++){
        if (*it == trx_id){
            return true;
        }
    }

    return false;
}

/*
 * Allocate and append a new lock object to the lock list
 * of the record having the key
 */
lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode){
    pthread_mutex_lock(&lock_manager_latch);

    lock_t * lock_obj, * working_lock_obj;
    lock_entry_t lock_entry;
    unordered_map<lock_key_t, lock_entry_t, hash_pair>::iterator it;

    lock_obj = (lock_t *)malloc(sizeof(lock_t));
    if (lock_obj == NULL){
        pthread_mutex_unlock(&lock_manager_latch);
        return nullptr;
    }

    it = lock_table.find(make_pair(table_id, key));

    // Case 1
    // if there is a predecessor's lock object in the lock list,
    // sleep until the predecessor to release its lock.
    if (it != lock_table.end()){
        working_lock_obj = it->second.head;

        while(!working_lock_obj->is_waiting){
            // Case 1 - 1
            // if there is already a lock_obj of same transaction in working list
            if (working_lock_obj->owner_trx_id == trx_id) {
                // Case 1 - 1 - 1
                // if the working lock_obj is shared mode,
                if (working_lock_obj->lock_mode == LOCK_SHARED) {
                    // Case 1 - 1 - 1 - 1
                    // S(working, mine) - S(waiting, mine)
                    // there's no need to acquire new lock for lock_obj
                    if (lock_mode == LOCK_SHARED) {
                        free(lock_obj);
                        pthread_mutex_unlock(&lock_manager_latch);
                        return working_lock_obj;
                    }

                    // Case 1 - 1 - 1 - 2
                    // S(working, mine) - X(waiting, mine)
                    else if (lock_mode == LOCK_EXCLUSIVE) {
                        // Case 1 - 1 - 1 - 2 - 1
                        // if there is only my trx's lock in working list
                        if (it->second.head == working_lock_obj
                            && working_lock_obj->next == NULL
                            && working_lock_obj->next->is_waiting) {
                            working_lock_obj->lock_mode = LOCK_EXCLUSIVE;
                            free(lock_obj);
                            pthread_mutex_unlock(&lock_manager_latch);
                            return working_lock_obj;
                        }
                        // if there is other trx's lock in working list
                        else if (!it->second.tail->is_waiting){
                            init_new_lock(table_id, key, trx_id, lock_mode, lock_obj);
                            lock_obj->prev = it->second.tail;
                            lock_obj->is_waiting = true;

                            lock_table[make_pair(table_id, key)].tail->next = lock_obj;
                            lock_table[make_pair(table_id, key)].tail = lock_obj;

                            trx_link_lock(lock_obj);

                            // deadlock may exist
                            if (check_deadlock(trx_id, lock_obj)) {
                                pthread_mutex_unlock(&lock_manager_latch);
                                return nullptr;
                            }

                            pthread_cond_wait(&(lock_obj->cond), &lock_manager_latch);

                            pthread_mutex_unlock(&lock_manager_latch);

                            return lock_obj;
                        }
                    }
                }
                // Case 1 - 1 - 2
                // X(working, mine) - S(waiting, mine)
                // X(working, mine) - X(waiting, mine)
                // if the working lock_obj is exclusive mode,
                // there's no need to acquire new lock for lock_obj
                else if (working_lock_obj->lock_mode == LOCK_EXCLUSIVE) {
                    free(lock_obj);
                    pthread_mutex_unlock(&lock_manager_latch);
                    return working_lock_obj;
                }
            }

            if (working_lock_obj->next == NULL) break;
            working_lock_obj = working_lock_obj->next;
        }
        // Case 1 - 2
        // if there is no lock_obj of same transaction in working list
        init_new_lock(table_id, key, trx_id, lock_mode, lock_obj);
        lock_obj->prev = it->second.tail;
        lock_obj->is_waiting = false;

        lock_table[make_pair(table_id, key)].tail->next = lock_obj;
        lock_table[make_pair(table_id, key)].tail = lock_obj;

        trx_link_lock(lock_obj);

        // Case 1 - 2 - 1
        // if the prev of lock_obj is working
        if (!lock_obj->prev->is_waiting){
            // Case 1 - 2 - 1 - 1
            // if S(working, other) is prev
            if (lock_obj->prev->lock_mode == LOCK_SHARED){
                // Case 1 - 2 - 1 - 1 - 1
                // if lock_obj is shared mode, it can acquire lock
                // S(working, other) - S(working, mine),
                if (lock_obj->lock_mode == LOCK_SHARED){
                    lock_obj->is_waiting = false;
                }
                // Case 1 - 2 - 1 - 1 - 2
                // if lock_obj is exclusive mode, it should wait
                // S(working, other) - X(waiting, mine),
                else if (lock_obj->lock_mode == LOCK_EXCLUSIVE){
                    lock_obj->is_waiting = true;
                    if (check_deadlock(trx_id, lock_obj)){
                        pthread_mutex_unlock(&lock_manager_latch);
                        return nullptr;
                    }
                    pthread_cond_wait(&(lock_obj->cond), &lock_manager_latch);
                }
            }
            // Case 1 - 2 - 1 - 2
            // if X(working, other) is prev
            else if (lock_obj->prev->lock_mode == LOCK_EXCLUSIVE){
                // regardless of whether lock_obj is exclusive or shared mode,
                // lock_obj should wait for the previous which is exclusive
                // X(working, other) - S(waiting, mine)
                // X(working, other) - X(waiting, mine)
                lock_obj->is_waiting = true;
                if (check_deadlock(trx_id, lock_obj)){
                    pthread_mutex_unlock(&lock_manager_latch);
                    return nullptr;
                }
                pthread_cond_wait(&(lock_obj->cond), &lock_manager_latch);
            }
        }
        // Case 1 - 2 - 2
        // if the prev of lock_obj is waiting
        else{
            lock_obj->is_waiting = true;
            if (check_deadlock(trx_id, lock_obj)){
                pthread_mutex_unlock(&lock_manager_latch);
                return nullptr;
            }
            pthread_cond_wait(&(lock_obj->cond), &lock_manager_latch);
        }

        pthread_mutex_unlock(&lock_manager_latch);
        return lock_obj;
    }
    // Case 2
    // if there is no predecessor's lock object,
    // return the address of the new lock object.
    else{
        lock_entry.table_id = table_id;
        lock_entry.key = key;
        lock_entry.tail = lock_obj;
        lock_entry.head = lock_obj;

        lock_table[make_pair(table_id, key)] = lock_entry;

        init_new_lock(table_id, key, trx_id, lock_mode, lock_obj);
        lock_obj->prev = NULL;
        lock_obj->is_waiting = false;

        trx_link_lock(lock_obj);
        pthread_mutex_unlock(&lock_manager_latch);
        return lock_obj;
    }
}

/*
 * Remove the lock_obj from the lock list
 */
int lock_release(lock_t * lock_obj){
    pthread_mutex_lock(&lock_manager_latch);
    lock_t * tmp_lock_obj;
    bool is_entry_erased = false;

    // send signal only when the lock_obj is the head of lock list
    // and the next of lock_obj is waiting for the lock

    // Case 1
    // if the lock_obj is the head of lock list
    if (lock_obj->sentinel->head == lock_obj){
        // Case 1 - 1
        // if there is a successor's lock waiting for the thread
        // releasing the lock, wake up the successor.
        if (lock_obj->next != NULL){
            lock_obj->sentinel->head = lock_obj->next;
            lock_obj->next->prev = NULL;

            // Case 1 - 1 - 1
            // if the next of lock_obj is waiting for the lock
            if (lock_obj->next->is_waiting){
                // Case 1 - 1 - 1 - 1
                // if the next of lock_obj is shared mode,
                if (lock_obj->next->lock_mode == LOCK_SHARED){
                    // Case 1 - 1 - 1 - 2 - 1
                    // when lock_obj is lock mode,
                    // just release the lock
                    if (lock_obj->lock_mode == LOCK_SHARED){
                        // do nothing
                    }
                    // Case 1 - 1 - 1 - 2 - 2
                    // when lock_obj is exclusive mode,
                    // wake up all the next shared mode of lock_obj
                    else if (lock_obj->lock_mode == LOCK_EXCLUSIVE){
                        tmp_lock_obj = lock_obj->next;
                        while(tmp_lock_obj != NULL && tmp_lock_obj->lock_mode == LOCK_SHARED) {
                            tmp_lock_obj->is_waiting = false;
                            pthread_cond_signal(&(tmp_lock_obj->cond));
                            tmp_lock_obj = tmp_lock_obj->next;
                        }
                    }
                }
                // Case 1 - 1 - 1 - 2
                // if the next of lock_obj is exclusive mode, wake it up
                else if (lock_obj->next->lock_mode == LOCK_EXCLUSIVE){
                    lock_obj->next->is_waiting = false;
                    pthread_cond_signal(&(lock_obj->next->cond));
                }
            }
        }
        // Case 1 - 2
        // if there is no more lock in the list
        else{
            lock_table.erase(lock_table.find(make_pair(lock_obj->sentinel->table_id, lock_obj->sentinel->key)));
            is_entry_erased = true;
        }
    }
    // Case 2
    // if the lock_obj is not the head of lock list
    else{
        // Case 2 - 1
        // if the lock_obj is not the tail of lock list,
        // relink the around lock_objs of it
        if (lock_obj->sentinel->tail != lock_obj){
            lock_obj->next->prev = lock_obj->prev;
            lock_obj->prev->next = lock_obj->next;

            if (!lock_obj->prev->is_waiting && lock_obj->prev->lock_mode == LOCK_SHARED){
                if (lock_obj->next->is_waiting && lock_obj->next->lock_mode == LOCK_SHARED){
                    tmp_lock_obj = lock_obj->next;

                    while(tmp_lock_obj != NULL && tmp_lock_obj->lock_mode == LOCK_SHARED){
                        tmp_lock_obj->is_waiting = false;
                        pthread_cond_signal(&tmp_lock_obj->cond);
                        tmp_lock_obj = tmp_lock_obj->next;
                    }
                }
            }
        }
        // Case 2 - 2
        // if the lock_obj is the tail of lock list,
        // update the tail and the prev of lock_obj
        else{
            lock_obj->sentinel->tail = lock_obj->prev;
            lock_obj->prev->next = NULL;
        }
    }

    if (!is_entry_erased){
        if (lock_obj->sentinel->head->next != NULL && lock_obj->sentinel->head->next->is_waiting
        && lock_obj->sentinel->head->owner_trx_id == lock_obj->sentinel->head->next->owner_trx_id){
            // merge
            tmp_lock_obj = lock_obj->sentinel->head;

            lock_obj->sentinel->head = lock_obj->sentinel->head->next;
            lock_obj->sentinel->head->is_waiting = false;
            lock_obj->sentinel->head->prev = NULL;

            trx_unlink_lock(tmp_lock_obj);
            free(tmp_lock_obj);
            pthread_cond_signal(&(lock_obj->sentinel->head->cond));
        }
    }

    free(lock_obj);

    pthread_mutex_unlock(&lock_manager_latch);
    return 0;
}