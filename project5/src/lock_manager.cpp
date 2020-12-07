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

void init_new_lock(int table_id, k_t key, int trx_id, int lock_mode, lock_t * lock_obj){
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

/*
 * Allocate and append a new lock object to the lock list
 * of the record having the key
 */
lock_t* lock_acquire(int table_id, k_t key, int trx_id, int lock_mode){
    pthread_mutex_lock(&lock_manager_latch);
    printf("     lock_acquire table_id %d, key %ld, trx_id %d, lock_mode %d\n", table_id, key, trx_id, lock_mode);

    lock_t * lock_obj, * working_lock_obj;
    lock_entry_t lock_entry;
    unordered_map<lock_key_t, lock_entry_t, hash_pair>::iterator it;

    lock_obj = (lock_t *)malloc(sizeof(lock_t));
    if (lock_obj == NULL){
        pthread_mutex_unlock(&lock_manager_latch);
        return nullptr;
    }

    it = lock_table.find(make_pair(table_id, key));

    // Case 2
    // if there is no predecessor's lock object,
    // return the address of the new lock object.
    if (it == lock_table.end()) {
        lock_entry.table_id = table_id;
        lock_entry.key = key;
        lock_entry.tail = lock_obj;
        lock_entry.head = lock_obj;

        lock_table[make_pair(table_id, key)] = lock_entry;

        lock_obj->next = NULL;
        lock_obj->prev = NULL;
        lock_obj->trx_prev_lock = NULL;
        lock_obj->trx_next_lock = NULL;
        lock_obj->sentinel = &lock_table[make_pair(table_id, key)];
        pthread_cond_init(&(lock_obj->cond), NULL);
        lock_obj->lock_mode = lock_mode;
        lock_obj->owner_trx_id = trx_id;
        lock_obj->is_waiting = false;

        trx_link_lock(lock_obj);
        pthread_mutex_unlock(&lock_manager_latch);
        return lock_obj;
    }
    // Case 1
    // if there is a predecessor's lock object in the lock list,
    // sleep until the predecessor to release its lock.
    else{
        bool has_mine = false;
        working_lock_obj = it->second.head;

        while(working_lock_obj != NULL && !working_lock_obj->is_waiting){
            if (working_lock_obj->owner_trx_id == trx_id){
                has_mine = true;
                break;
            }

            working_lock_obj = working_lock_obj->next;
        }

        // Case : there is lock_obj of same transaction in working list
        if (has_mine){
            //   X(working, mine) - S(waiting, me)
            //   X(working, mine) - X(waiting, me)
            // there's no need to acquire new lock for lock_obj
            if (working_lock_obj->lock_mode == LOCK_EXCLUSIVE) {
                free(lock_obj);
                pthread_mutex_unlock(&lock_manager_latch);
                return working_lock_obj;
            }
            // S(working, mine) - S(waiting, me)
            // S(working, mine) - X(waiting, me)
            else if (working_lock_obj->lock_mode == LOCK_SHARED) {
                // S(working, mine) - S(waiting, me)
                // there's no need to acquire new lock for lock_obj
                if (lock_mode == LOCK_SHARED) {
                    free(lock_obj);
                    pthread_mutex_unlock(&lock_manager_latch);
                    return working_lock_obj;
                }
                // S(working, mine) - X(waiting, me)
                else if (lock_mode == LOCK_EXCLUSIVE) {
                    // if there is only my trx's lock in working list,
                    // upgrade the mode to X mode
                    if (it->second.head == working_lock_obj) {
                        if (working_lock_obj->next == NULL
                            || working_lock_obj->next->is_waiting) {
                            working_lock_obj->lock_mode = LOCK_EXCLUSIVE;
                            free(lock_obj);
                            pthread_mutex_unlock(&lock_manager_latch);
                            return working_lock_obj;
                        }
                    }
                    // if there is other trx's lock in working list,
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
                    else{
                        pthread_mutex_unlock(&lock_manager_latch);
                        return nullptr;
                    }
                }
            }
        }
        // Case : no lock_obj of same transaction in working list
        //        1. append new lock_obj to the end of lock list
        //        2. 1) wait if previous lock_obj is waiting
        //           2) wait if previous lock_obj is working exclusive lock
        //           3) acquire if previous lock_obj is working shared lock
        //        3. check deadlock
        //           1) if exist, return null (and do abort)
        else{
            // append new lock_obj to the end of lock list
            lock_obj->next = NULL;
            lock_obj->prev = it->second.tail;
            lock_obj->trx_prev_lock = NULL;
            lock_obj->trx_next_lock = NULL;
            lock_obj->sentinel = &lock_table[make_pair(table_id, key)];
            pthread_cond_init(&(lock_obj->cond), NULL);
            lock_obj->lock_mode = lock_mode;
            lock_obj->owner_trx_id = trx_id;
            lock_obj->is_waiting = false;

            lock_table[make_pair(table_id, key)].tail->next = lock_obj;
            lock_table[make_pair(table_id, key)].tail = lock_obj;

            // if previous lock_obj is waiting
            // let new lock_obj wait
            // and link to its trx table if deadlock doesn't exist
            if (lock_obj->prev->is_waiting){
                lock_obj->is_waiting = true;
                if (check_deadlock(trx_id, lock_obj)){
                    // free(lock_obj);
                    pthread_mutex_unlock(&lock_manager_latch);
                    return nullptr;
                }
                trx_link_lock(lock_obj);

                pthread_cond_wait(&(lock_obj->cond), &lock_manager_latch);
            }
            // if previous lock_obj is working,
            else{
                //   S(working, other) - S(working, me)
                // it can acquire lock
                // and there's no need to check deadlock so just link to its trx table
                if (lock_obj->prev->lock_mode == LOCK_SHARED && lock_obj->lock_mode == LOCK_SHARED){
                    lock_obj->is_waiting = false;
                    trx_link_lock(lock_obj);
                }
                //   S(working, other) - X(waiting, me)
                //   X(working, other) - S(waiting, me)
                //   X(working, other) - X(waiting, me)
                // lock_obj should wait until previous lock releases
                // and link to its trx table if deadlock doesn't exist
                else{
                    lock_obj->is_waiting = true;
                    if (check_deadlock(trx_id, lock_obj)){
                        // free(lock_obj);
                        pthread_mutex_unlock(&lock_manager_latch);
                        return nullptr;
                    }
                    trx_link_lock(lock_obj);

                    pthread_cond_wait(&(lock_obj->cond), &lock_manager_latch);
                }
            }

            pthread_mutex_unlock(&lock_manager_latch);
            return lock_obj;
        }
    }
}

/*
 * Remove the lock_obj from the lock list
 */
int lock_release(lock_t * lock_obj){
    pthread_mutex_lock(&lock_manager_latch);
    printf("     lock_release table_id %d, key %ld, trx_id %d, lock_mode %d\n",
           lock_obj->sentinel->table_id, lock_obj->sentinel->key, lock_obj->owner_trx_id, lock_obj->lock_mode);
    lock_t * tmp_lock_obj;

    // send signal only when the lock_obj is the head of lock list
    // and the next of lock_obj is waiting for the lock

    // if the lock_obj is the head of lock list
    if (lock_obj->sentinel->head == lock_obj){
        // if there is no more lock in the list,
        // just erase the lock.
        if (lock_obj->sentinel->tail == lock_obj){
            lock_table.erase(lock_table.find(make_pair(lock_obj->sentinel->table_id, lock_obj->sentinel->key)));
            free(lock_obj);
            pthread_mutex_unlock(&lock_manager_latch);
            return 0;
        }

        // if there is a successor's lock waiting for the thread releasing the lock,
        // wake up the successor.
        lock_obj->sentinel->head = lock_obj->next;
        lock_obj->sentinel->head->prev = NULL;

        // if the next of lock_obj is exclusive mode, wake it up
        if (lock_obj->next->lock_mode == LOCK_EXCLUSIVE){
            lock_obj->next->is_waiting = false;
            pthread_cond_signal(&(lock_obj->next->cond));
        }
        else if (lock_obj->next->lock_mode == LOCK_SHARED){
            if (lock_obj->lock_mode == LOCK_SHARED){
                // do nothing
            }
            else if (lock_obj->lock_mode == LOCK_EXCLUSIVE){
                // when lock_obj is exclusive mode,
                // wake up all the next shared mode of lock_obj
                tmp_lock_obj = lock_obj->next;
                while(tmp_lock_obj != NULL && tmp_lock_obj->lock_mode == LOCK_SHARED) {
                    tmp_lock_obj->is_waiting = false;
                    pthread_cond_signal(&(tmp_lock_obj->cond));
                    tmp_lock_obj = tmp_lock_obj->next;
                }
            }
        }
    }
    // Case 2
    // if the lock_obj is not the head of lock list
    else{
        // if the lock_obj is the tail of lock list,
        // update the tail.
        if (lock_obj->sentinel->tail == lock_obj){
            lock_obj->sentinel->tail = lock_obj->prev;
            lock_obj->sentinel->tail->next = NULL;
        }
        // if the lock_obj is not the tail of lock list,
        // relink the around lock_objs of it
        else {
            lock_obj->next->prev = lock_obj->prev;
            lock_obj->prev->next = lock_obj->next;

//            if (!lock_obj->prev->is_waiting && lock_obj->prev->lock_mode == LOCK_SHARED){
//                if (lock_obj->next->is_waiting && lock_obj->next->lock_mode == LOCK_SHARED){
//                    tmp_lock_obj = lock_obj->next;
//
//                    while(tmp_lock_obj != NULL && tmp_lock_obj->lock_mode == LOCK_SHARED){
//                        tmp_lock_obj->is_waiting = false;
//                        pthread_cond_signal(&tmp_lock_obj->cond);
//                        tmp_lock_obj = tmp_lock_obj->next;
//                    }
//                }
//            }
        }
    }

    if (lock_obj->sentinel->head->next != NULL && lock_obj->sentinel->head->next->is_waiting
    && lock_obj->sentinel->head->owner_trx_id == lock_obj->sentinel->head->next->owner_trx_id){
        // merge
        tmp_lock_obj = lock_obj->sentinel->head;
        trx_unlink_lock(tmp_lock_obj);
        free(tmp_lock_obj);

        lock_obj->sentinel->head = lock_obj->sentinel->head->next;
        lock_obj->sentinel->head->is_waiting = false;
        lock_obj->sentinel->head->prev = NULL;

        pthread_cond_signal(&(lock_obj->sentinel->head->cond));
    }

    free(lock_obj);
    pthread_mutex_unlock(&lock_manager_latch);
    return 0;

}

void print_lock_table_after_acquire(void){
    pthread_mutex_lock(&lock_manager_latch);
    unordered_map <lock_key_t, lock_entry_t, hash_pair>::iterator it;

    int table_id;
    k_t key;
    lock_entry_t lock_entry;
    lock_t * lock_obj;

    printf("\n     ****************** AFTER ACQUIRE LOCK *****************************\n");
    for(it = lock_table.begin(); it != lock_table.end(); it++){
        table_id = it->first.first;
        key = it->first.second;
        lock_entry = it->second;
        lock_obj = lock_entry.head;

        printf("     LOCK [%2d, %2ld] : ", table_id, key);

        while(lock_obj != NULL){
            printf("(trx_id %d, %c, %s) -> ", lock_obj->owner_trx_id,
                   lock_obj->lock_mode == LOCK_SHARED ? 'S' : 'X',
                   lock_obj->is_waiting ? "waiting" : "working");
            lock_obj = lock_obj->next;
        }
        printf("\n");
    }
    printf("     ****************************************************************\n\n");

    pthread_mutex_unlock(&lock_manager_latch);
}

void print_lock_table_after_release(void){
    pthread_mutex_lock(&lock_manager_latch);
    unordered_map <lock_key_t, lock_entry_t, hash_pair>::iterator it;

    int table_id;
    k_t key;
    lock_entry_t lock_entry;
    lock_t * lock_obj;
    printf("\n     ****************** AFTER RELEASE LOCK ******************\n");
    for(it = lock_table.begin(); it != lock_table.end(); it++){
        table_id = it->first.first;
        key = it->first.second;
        lock_entry = it->second;
        lock_obj = lock_entry.head;

        printf("     LOCK [%2d, %2ld] : ", table_id, key);

        while(lock_obj != NULL){
            printf("(trx_id %d, %c, %s) -> ", lock_obj->owner_trx_id,
                   lock_obj->lock_mode == LOCK_SHARED ? 'S' : 'X',
                   lock_obj->is_waiting ? "waiting" : "working");
            lock_obj = lock_obj->next;
        }
        printf("\n");
    }
    printf("     *****************************************************\n\n");

    pthread_mutex_unlock(&lock_manager_latch);
}