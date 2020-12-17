#ifndef TRANSACTION_MANAGER_H
#define TRANSACTION_MANAGER_H

#include <stdint.h>
#include <pthread.h>
#include <unordered_map>
#include <iostream>
#include <set>
#include <stack>

#include "file.h"
#include "lock_manager.h"
#include "log_manager.h"

using namespace std;

typedef struct pair<int, k_t> log_key_t;
typedef struct undo_log_t{
    lsn_t lsn;
    int table_id;
    pagenum_t pagenum;
    k_t key;
    int key_idx;
    char old_value[DATA_LENGTH];
    char new_value[DATA_LENGTH];
}undo_log_t;

typedef struct trx_entry_t{
    int trx_id;
    lock_t * head;
    lock_t * tail;
    stack <undo_log_t> undo_log_st;
    pthread_mutex_t trx_latch;
}trx_entry_t;

/*
 * Allocate a transaction structure and initialize it
 */
int trx_begin(void);

/*
 * Clean up the transaction with given trx_id and its related information
 * that has been used in your lock manager.
 */
int trx_commit(int trx_id);

bool check_trx_abort(int trx_id);
void trx_abort(int trx_id);

/*
 * Link new lock_obj to the transaction entry of transaction manager
 */
void trx_link_lock(lock_t * lock_obj);

/*
 * Unlink lock_obj to the transaction entry of transaction manager
 */
void trx_unlink_lock(lock_t * lock_obj);

void trx_write_log(lock_t * lock_obj, pagenum_t pagenum, int key_idx, lsn_t lsn, char * old_value, char * new_value);

void get_wait_for_graph(lock_t * lock_obj, set<int> & visit_set);

bool check_deadlock(int trx_id, lock_t * lock_obj);


void print_transaction();


#endif //TRANSACTION_MANAGER_H
