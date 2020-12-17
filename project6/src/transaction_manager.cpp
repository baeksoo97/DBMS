#include "transaction_manager.h"
#include "index.h"
#include "log_manager.h"

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
    // printf("TRX BEGIN %d \n", trx_cnt);
    trx.head = NULL;
    trx.tail = NULL;
    pthread_mutex_init(&trx.trx_latch, NULL);

    trx_manager[trx.trx_id] = trx;

    log_write_general_record(LOG_TYPE_BEGIN, trx.trx_id);
//    print_transaction();
    pthread_mutex_unlock(&trx_manager_latch);

    return trx.trx_id;
}

/*
 * Clean up the transaction with given trx_id and its related information
 * that has been used in your lock manager.
 */
int trx_commit(int trx_id){
    // printf("TRX COMMIT %d\n", trx_id);
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
        // print_lock_table_after_release();
    }
    release_lock_latch();

    pthread_mutex_lock(&trx_manager_latch);
    trx_manager.erase(trx_id);
    pthread_mutex_unlock(&trx_manager_latch);

    log_write_general_record(LOG_TYPE_COMMIT, trx_id);

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
    lsn_t next_undo_lsn, compensate_lsn;
    k_t key;
    stack <undo_log_t> undo_log_st;
    undo_log_t log;

    pthread_mutex_lock(&trx_manager_latch);
    if (trx_manager.count(trx_id) == 0){
        pthread_mutex_unlock(&trx_manager_latch);
        return;
    }

    // undo operation using log_map
    // & clear log_map

    undo_log_st = trx_manager[trx_id].undo_log_st;
    while(!undo_log_st.empty()){
        log = undo_log_st.top();
        undo_log_st.pop();

        table_id = log.table_id;
        key = log.key;
        next_undo_lsn = undo_log_st.empty() ? -1 : undo_log_st.top().lsn;
        compensate_lsn = log_write_compensate_record(trx_id, table_id, log.pagenum,
                                                     PAGE_HEADER_SIZE + DATA_LENGTH * log.key_idx,
                                                     log.old_value, log.new_value, next_undo_lsn);
        undo(table_id, log.pagenum, key, log.old_value, compensate_lsn);
    }

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
    trx_manager.erase(trx_id);
    pthread_mutex_unlock(&trx_manager_latch);

    log_write_general_record(LOG_TYPE_ROLLBACK, trx_id);

    log_write_disk();
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

void trx_write_log(lock_t * lock_obj, pagenum_t pagenum, int key_idx, lsn_t lsn, char * old_value, char * new_value){
    undo_log_t log;

    log.lsn = lsn;
    log.table_id = lock_obj->sentinel->table_id;
    log.pagenum = pagenum;
    log.key = lock_obj->sentinel->key;
    log.key_idx = key_idx;
    strcpy(log.old_value, old_value);
    strcpy(log.new_value, new_value);

    pthread_mutex_lock(&trx_manager_latch);
    trx_manager[lock_obj->owner_trx_id].undo_log_st.push(log);
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

    if (visit_set.count(trx_id)){
        pthread_mutex_unlock(&trx_manager_latch);
        return true;
    }

    pthread_mutex_unlock(&trx_manager_latch);
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