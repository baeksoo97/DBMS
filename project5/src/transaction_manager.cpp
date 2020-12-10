#include "transaction_manager.h"
#include "index.h"

unordered_map <int, trx_entry_t> trx_manager;
pthread_mutex_t trx_manager_latch = PTHREAD_MUTEX_INITIALIZER;
int trx_cnt = 0;

/*
 * Allocate a transaction structure and initialize it
 */
int trx_begin(void){
    pthread_mutex_lock(&trx_manager_latch);

    trx_entry_t trx;
    trx.trx_id = ++trx_cnt;
    printf("TRX BEGIN %d \n", trx_cnt);
    trx.head = NULL;
    trx.tail = NULL;

    trx_manager[trx.trx_id] = trx;

//    print_transaction();
    pthread_mutex_unlock(&trx_manager_latch);

    return trx.trx_id;
}

/*
 * Clean up the transaction with given trx_id and its related information
 * that has been used in your lock manager.
 */
int trx_commit(int trx_id){
    printf("TRX COMMIT %d\n", trx_id);
    lock_t * lock_obj, * tmp_lock_obj;

    if (trx_id <= 0){
        perror("ERROR TRX_COMMIT");
        return -1;
    }

    pthread_mutex_lock(&trx_manager_latch);
    lock_obj = trx_manager[trx_id].head;
    pthread_mutex_unlock(&trx_manager_latch);

    acquire_lock_latch();
//    print_transaction();
    while(lock_obj != NULL){
        tmp_lock_obj = lock_obj;
        lock_obj = lock_obj->trx_next_lock;
        lock_release(tmp_lock_obj); // strict two phase lock
        print_lock_table_after_release();
    }
    release_lock_latch();

    pthread_mutex_lock(&trx_manager_latch);
    trx_manager[trx_id].log_map.clear();
    trx_manager.erase(trx_id);
    pthread_mutex_unlock(&trx_manager_latch);

    return trx_id;
}

// return true if transaction is aborted
// return false if transaction is not aborted
bool check_trx_abort(int trx_id){
    pthread_mutex_lock(&trx_manager_latch);
    if (trx_manager.count(trx_id) == 0) {
        pthread_mutex_unlock(&trx_manager_latch);
        return true;
    }
    else {
        pthread_mutex_unlock(&trx_manager_latch);
        return false;
    }
}

void trx_abort(int trx_id){
    lock_t * lock_obj, * tmp_lock_obj;
    int table_id;
    k_t key;
    unordered_map<log_key_t, log_t, hash_pair>::iterator it;

    pthread_mutex_lock(&trx_manager_latch);
    if (trx_manager.count(trx_id) == 0){
        pthread_mutex_unlock(&trx_manager_latch);
        return;
    }

    // undo operation using log_map
    // & clear log_map
    for(it = trx_manager[trx_id].log_map.begin(); it != trx_manager[trx_id].log_map.end(); it++){
        table_id = it->first.first;
        key = it->first.second;
        undo(table_id, key, it->second.old_value);
    }

    trx_manager[trx_id].log_map.clear();

    // release all locks in transaction
    // & clear trx entry
    lock_obj = trx_manager[trx_id].head;

    pthread_mutex_unlock(&trx_manager_latch);

    acquire_lock_latch();
    while(lock_obj != NULL){
        tmp_lock_obj = lock_obj;
        lock_obj = lock_obj->trx_next_lock;
        lock_release_for_abort(tmp_lock_obj); // strict two phase lock
    }
    release_lock_latch();

    pthread_mutex_lock(&trx_manager_latch);
    trx_manager[trx_id].log_map.clear();
    trx_manager.erase(trx_id);
    pthread_mutex_unlock(&trx_manager_latch);
}

/*
 * Link new lock_obj to the transaction entry of transaction manager
 */
void trx_link_lock(lock_t * lock_obj){
    int trx_id;
    pthread_mutex_lock(&trx_manager_latch);

    trx_id = lock_obj->owner_trx_id;

    if (trx_manager.count(trx_id) == 0){
        printf("ERROR TRX_LINK_LOCK : there's no trx %d in manager\n", trx_id);
        pthread_mutex_unlock(&trx_manager_latch);
        return;
    }

    // if the lock_obj is the first operation
    if (trx_manager[trx_id].head == NULL){
        trx_manager[trx_id].head = lock_obj;
        trx_manager[trx_id].tail = lock_obj;
    }
    // if there are already other operations in trx entry,
    // update the lock_obj and the tail
    else{
        lock_obj->trx_prev_lock = trx_manager[trx_id].tail;
        trx_manager[trx_id].tail->trx_next_lock = lock_obj;
        trx_manager[trx_id].tail = lock_obj;
    }

    pthread_mutex_unlock(&trx_manager_latch);
}

/*
 * Unlink lock_obj to the transaction entry of transaction manager
 */
void trx_unlink_lock(lock_t * lock_obj){
    pthread_mutex_lock(&trx_manager_latch);
    int trx_id;

    trx_id = lock_obj->owner_trx_id;

    // if the lock_obj is the head of the trx entry,
    // update the head of trx entry
    if (trx_manager[trx_id].head == lock_obj && trx_manager[trx_id].tail == lock_obj){
        trx_manager[trx_id].head = NULL;
        trx_manager[trx_id].tail = NULL;
    }
    else if (trx_manager[trx_id].head == lock_obj){
        trx_manager[trx_id].head = lock_obj->trx_next_lock;
        trx_manager[trx_id].head->trx_prev_lock= NULL;
    }
    else if (trx_manager[trx_id].tail == lock_obj){
        trx_manager[trx_id].tail = lock_obj->trx_prev_lock;
        lock_obj->trx_prev_lock->trx_next_lock = NULL;
    }
    else{
        lock_obj->trx_prev_lock->trx_next_lock = lock_obj->trx_next_lock;
        lock_obj->trx_next_lock->trx_prev_lock = lock_obj->trx_prev_lock;
    }
    pthread_mutex_unlock(&trx_manager_latch);
}

void trx_write_log(lock_t * lock_obj, char * old_value){
    int table_id, trx_id;
    k_t key;
    log_t log;

    trx_id = lock_obj->owner_trx_id;
    table_id = lock_obj->sentinel->table_id;
    key = lock_obj->sentinel->key;

    pthread_mutex_lock(&trx_manager_latch);
    if (trx_manager[trx_id].log_map.find(make_pair(table_id, key)) == trx_manager[trx_id].log_map.end()){
        log.table_id = table_id;
        log.key = key;
        strcpy(log.old_value, old_value);

        trx_manager[trx_id].log_map[make_pair(table_id, key)] = log;
    }

    pthread_mutex_unlock(&trx_manager_latch);
}

void get_wait_for_graph(lock_t * lock_obj, set<int> &visit_set){
    int prev_trx_id;
    lock_t * prev_lock_obj, * tmp_lock_obj;

    // don't need to check dependency when lock is working
    if (!lock_obj->is_waiting)
        return;

    prev_lock_obj = lock_obj->prev;

    if (prev_lock_obj == NULL){
        printf("get_wait_for_graph prev lock is null\n");
        return;
    }

    if (prev_lock_obj->lock_mode == LOCK_SHARED){
        if (prev_lock_obj->is_waiting){
            while (prev_lock_obj != NULL){
                if (prev_lock_obj->lock_mode == LOCK_EXCLUSIVE)
                    break;

                prev_trx_id = prev_lock_obj->owner_trx_id;
                if (visit_set.count(prev_trx_id) == 0){
                    visit_set.insert(prev_trx_id);

                    tmp_lock_obj = trx_manager[prev_trx_id].head;
                    while(tmp_lock_obj != NULL){
                        get_wait_for_graph(tmp_lock_obj, visit_set);
                        tmp_lock_obj = tmp_lock_obj->trx_next_lock;
                    }
                }
                prev_lock_obj = prev_lock_obj->prev;
            }
        }
        else{
            while (prev_lock_obj != NULL){
                if (prev_lock_obj->owner_trx_id != lock_obj->owner_trx_id){
                    prev_trx_id = prev_lock_obj->owner_trx_id;
                    if (visit_set.count(prev_trx_id) == 0){
                        visit_set.insert(prev_trx_id);

                        tmp_lock_obj = trx_manager[prev_trx_id].head;
                        while(tmp_lock_obj != NULL){
                            get_wait_for_graph(tmp_lock_obj, visit_set);
                            tmp_lock_obj = tmp_lock_obj->trx_next_lock;
                        }
                    }
                    break;
                }

                prev_lock_obj = prev_lock_obj->prev;
            }
        }
    }
    else if (prev_lock_obj->lock_mode == LOCK_EXCLUSIVE){
        prev_trx_id = prev_lock_obj->owner_trx_id;
        if (visit_set.count(prev_trx_id)){
            return;
        }
        visit_set.insert(prev_trx_id);

        tmp_lock_obj = trx_manager[prev_trx_id].head;
        while(tmp_lock_obj != NULL){
            get_wait_for_graph(tmp_lock_obj, visit_set);
            tmp_lock_obj = tmp_lock_obj->trx_next_lock;
        }
    }
}

bool check_deadlock(int trx_id, lock_t * lock_obj){
    set<int> visit_set;
    set<int>::iterator it;

    pthread_mutex_lock(&trx_manager_latch);
    get_wait_for_graph(lock_obj, visit_set);
    pthread_mutex_unlock(&trx_manager_latch);

    for(it = visit_set.begin(); it != visit_set.end(); it++){
        if (*it == trx_id){
            return true;
        }
    }

    return false;
}

void print_transaction(){
    unordered_map <int, trx_entry_t> ::iterator it;
    int trx_id;
    trx_entry_t trx_entry;
    lock_t * lock_obj;

    printf("     ****************** TRANSACTION ******************\n");
    for (it = trx_manager.begin(); it != trx_manager.end(); it++){
        trx_id = it->first;
        trx_entry = it->second;
        lock_obj = trx_entry.head;

        printf("     TRX %d -> ", trx_id);

        while(lock_obj != NULL){
            printf("(trx_id %d, %c, %s) -> ", lock_obj->owner_trx_id,
                   lock_obj->lock_mode == LOCK_SHARED ? 'S' : 'X',
                   lock_obj->is_waiting ? "waiting" : "working");
            lock_obj = lock_obj->trx_next_lock;
        }
        printf("\n");
    }
    printf("     ***********************************************\n");
}