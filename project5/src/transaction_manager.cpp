#include "transaction_manager.h"

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

    trx_manager[trx.trx_id] = trx;

    if (trx.trx_id == 0){
        perror("ERROR TRX_BEGIN");
        return -1;
    }

    pthread_mutex_unlock(&trx_manager_latch);

    return trx.trx_id;
}

/*
    Clean up the transaction with given trx_id and its related information
    that has been used in your lock manager.
*/
int trx_commit(int trx_id){
    pthread_mutex_lock(&trx_manager_latch);

    lock_t * lock, * tmp_lock;

    lock = trx_manager[trx_id].head;

    while(lock != NULL){
        tmp_lock = lock;
        lock = lock->trx_next_lock;
        lock_release(tmp_lock); // strict two phase lock
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

    lock_t * lock, * tmp_lock;

    lock = trx_manager[trx_id].head;


    pthread_mutex_unlock(&trx_manager_latch);
}

