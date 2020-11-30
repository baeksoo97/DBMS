#include "transaction_manager.h"
#include "index.h"

static unordered_map <int, trx_entry_t> trx_manager;
static pthread_mutex_t trx_manager_latch = PTHREAD_MUTEX_INITIALIZER;
static int trx_cnt = 0;

/*
 * Allocate a transaction structure and initialize it
 */
int trx_begin(void){
    pthread_mutex_lock(&trx_manager_latch);

    trx_entry_t trx;
    trx.trx_id = ++trx_cnt;
    trx.head = NULL;
    trx.is_aborted = false;

    trx_manager[trx.trx_id] = trx;

    if (trx.trx_id == 0){
        perror("ERROR TRX_BEGIN");
        return -1;
    }

    pthread_mutex_unlock(&trx_manager_latch);

    return trx.trx_id;
}

/*
 * Clean up the transaction with given trx_id and its related information
 * that has been used in your lock manager.
 */
int trx_commit(int trx_id){
    pthread_mutex_lock(&trx_manager_latch);

    lock_t * lock, * tmp_lock;

    lock = trx_manager[trx_id].head;

    while(lock != NULL){
        tmp_lock = lock;
        lock = lock->trx_next_lock;
        lock_release(tmp_lock, false); // strict two phase lock
    }

    trx_manager.erase(trx_id);

    if (trx_id == 0){
        perror("ERROR TRX_COMMIT");
        return -1;
    }

    pthread_mutex_unlock(&trx_manager_latch);

    return trx_id;
}

void trx_abort(int trx_id){
    pthread_mutex_lock(&trx_manager_latch);

    lock_t * lock_obj, * tmp_lock_obj;
    int table_id;
    int64_t key;
    char * old_value;
    unordered_map<log_key_t, log_t, hash_pair>::iterator it;

    // undo operation using log_map
    // & clear log_map
    for(it = trx_manager[trx_id].log_map.begin(); it != trx_manager[trx_id].log_map.end(); it++){
        table_id = it->first.first;
        key = it->first.second;
        strcpy(old_value, it->second.old_value);
        undo(table_id, key, old_value);
    }
    trx_manager[trx_id].log_map.clear();

    // release all locks in transaction
    // & clear trx entry
    lock_obj = trx_manager[trx_id].head;
    while(lock_obj != NULL){
        tmp_lock_obj = lock_obj;
        lock_release(tmp_lock_obj, true);
        lock_obj = lock_obj->trx_next_lock;
    }
    trx_manager.erase(trx_id);

    pthread_mutex_unlock(&trx_manager_latch);
    pthread_mutex_unlock(&lock_manager_latch);
}

/*
 * Link new lock_obj to the transaction entry of transaction manager
 */
void trx_link_lock(lock_t * lock_obj){
    int trx_id;

    trx_id = lock_obj->owner_trx_id;

    // if the lock_obj is the first operation in trx entry,
    // just link to the head of trx entry
    if (trx_manager[trx_id].head == NULL){
        trx_manager[trx_id].head = lock_obj;
    }
    // if there are already other operations in trx entry,
    // update the lock_obj and the tail
    else{
        lock_obj->trx_prev_lock = trx_manager[trx_id].tail;
        trx_manager[trx_id].tail->trx_next_lock = lock_obj;
    }

    // update the tail of trx entry
    trx_manager[trx_id].tail = lock_obj;
}


/*
 * Unlink lock_obj to the transaction entry of transaction manager
 */
void trx_unlink_lock(lock_t * lock_obj){
    int trx_id;

    trx_id = lock_obj->owner_trx_id;

    // if the lock_obj is the head of the trx entry,
    // update the head of trx entry
    if (trx_manager[trx_id].head == lock_obj){
        trx_manager[trx_id].head = lock_obj->trx_next_lock;
        lock_obj->trx_next_lock->prev = NULL;
    }
    else{
        lock_obj->trx_prev_lock->trx_next_lock = lock_obj->trx_next_lock;
        lock_obj->trx_next_lock->trx_prev_lock = lock_obj->trx_prev_lock;
    }
}

void trx_write_log(lock_t * lock_obj, char * old_value){
    int table_id, trx_id;
    int64_t key;
    log_t log;

    trx_id = lock_obj->owner_trx_id;
    table_id = lock_obj->sentinel->table_id;
    key = lock_obj->sentinel->key;

    if (trx_manager[trx_id].log_map.find(make_pair(table_id, key)) == trx_manager[trx_id].log_map.end()){
        log.table_id = table_id;
        log.key = key;
        strcpy(log.old_value, old_value);

        trx_manager[trx_id].log_map[make_pair(table_id, key)] = log;
    }
}

set<int> visit_trx(int trx_id, set<int> visit_set){
    lock_t * trx_lock_obj;

    if (visit_set.count(trx_id))
        return visit_set;

    visit_set.insert(trx_id);
    trx_lock_obj = trx_manager[trx_id].head;
    while (trx_lock_obj != NULL) {
        if (trx_lock_obj->is_waiting)
            visit_set = get_wait_for_graph(trx_lock_obj, visit_set);
        trx_lock_obj = trx_lock_obj->trx_next_lock;
    }

    return visit_set;
}

set<int> get_wait_for_graph(lock_t * lock_obj, set<int> visit_set){
    int trx_id;
    lock_t * tmp_lock_obj;

    if (!lock_obj->is_waiting){
        return visit_set;
    }

    tmp_lock_obj = lock_obj->prev;

    if (!tmp_lock_obj->is_waiting){
        if (tmp_lock_obj->lock_mode == LOCK_SHARED){
            while (tmp_lock_obj != NULL) {
                trx_id = tmp_lock_obj->owner_trx_id;
                if (trx_id != lock_obj->owner_trx_id) {
                    visit_set = visit_trx(trx_id, visit_set);
                }

                tmp_lock_obj = tmp_lock_obj->prev;
            }
        }
        else if (tmp_lock_obj->lock_mode == LOCK_EXCLUSIVE){
            trx_id = tmp_lock_obj->owner_trx_id;
            visit_set = visit_trx(trx_id, visit_set);
        }
    }
    else{
        if (tmp_lock_obj->lock_mode == LOCK_SHARED){
            while (tmp_lock_obj->is_waiting) {
                if (tmp_lock_obj->lock_mode == LOCK_EXCLUSIVE)
                    break;
                tmp_lock_obj = tmp_lock_obj->prev;
            }

            if (!tmp_lock_obj->is_waiting) {
                while (tmp_lock_obj != NULL) {
                    trx_id = tmp_lock_obj->owner_trx_id;
                    visit_set = visit_trx(trx_id, visit_set);
                    tmp_lock_obj = tmp_lock_obj->prev;
                }
            }
            else {
                visit_set = visit_trx(trx_id, visit_set);
            }
        }
        else if (tmp_lock_obj->lock_mode == LOCK_EXCLUSIVE){
            visit_set = visit_trx(trx_id, visit_set);
        }
    }

    return visit_set;

}