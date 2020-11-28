#ifndef TRANSACTION_MANAGER_H
#define TRANSACTION_MANAGER_H

#include "lock_manager.h"
#include <stdint.h>
#include <pthread.h>
#include <unordered_map>
#include <iostream>
using namespace std;

typedef struct trx_entry_t{
    int trx_id;
    lock_t * head;
}trx_entry_t;

// Allocate a transaction structure and initialize it
int trx_begin(void);

// Clean up the transaction with given trx_id and its related information
// that has been uesd in your lock manager
int trx_commit(int trx_id);


#endif //TRANSACTION_MANAGER_H
