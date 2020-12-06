#ifndef TRANSACTION_MANAGER_H
#define TRANSACTION_MANAGER_H

#include "file.h"
#include "lock_manager.h"
#include <stdint.h>
#include <pthread.h>
#include <unordered_map>
#include <iostream>
#include <set>
using namespace std;

typedef struct pair<int, k_t> log_key_t;
typedef struct log_t{
    int table_id;
    k_t key;
    char old_value[120];
}log_t;

typedef struct trx_entry_t{
    int trx_id;
    lock_t * head;
    lock_t * tail;
    unordered_map <log_key_t, log_t, hash_pair> log_map;
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

void trx_write_log(lock_t * lock_obj, char * old_value);

set<int> get_wait_for_graph(lock_t * lock_obj, set<int> visit_set);



#endif //TRANSACTION_MANAGER_H
