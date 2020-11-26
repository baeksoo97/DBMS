#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdint.h>
#include <pthread.h>
#include <unordered_map>
#include <iostream>
using namespace std;

#define LOCK_SHARED 0
#define LOCK_EXCLUSIVE 1

typedef struct hash_entry_t hash_entry_t;
typedef struct lock_t lock_t;
typedef pair<int, int64_t> hash_key_t;

typedef struct hash_entry_t{
    int table_id;
    int64_t record_id;
    lock_t * tail;
    lock_t * head;
}hash_entry_t;

typedef struct lock_t {
    lock_t * prev;
    lock_t * next;
    hash_entry_t * sentinel;
    pthread_cond_t cond;
    int lock_mode;
    lock_t * trx_next_lock;
    int owner_trx_id;
}lock_t;

typedef struct hash_pair {
    template <class T1, class T2>
    size_t operator() (const pair<T1, T2> & p) const{
        return hash<T1>()(p.first) ^ hash<T2>()(p.second);
    }
}hash_pair;

static unordered_map <hash_key_t, hash_entry_t, hash_pair> hash_table;
static unordered_map <int, lock_t> trx_manager;

static pthread_mutex_t lock_manager_latch;
static pthread_mutex_t trx_manager_latch;

/* APIs for lock table */
// Initialize any data structured required for implementing lock table
int init_lock_table();

// Allocate and append a new lock object to the lock list
// of the record having the key
lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode);

// Remove the lock_obj from the lock list
int lock_release(lock_t* lock_obj);

#endif //TRANSACTION_H
