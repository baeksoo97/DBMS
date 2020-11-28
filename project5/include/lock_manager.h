#ifndef LOCK_MANAGER_H
#define LOCK_MANAGER_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unordered_map>
using namespace std;

#define LOCK_SHARED 0
#define LOCK_EXCLUSIVE 1

typedef struct lock_entry_t lock_entry_t;
typedef struct lock_t lock_t;
typedef pair<int, int64_t> lock_key_t;

typedef struct lock_entry_t{
    int table_id;
    int64_t record_id;
    lock_t * tail;
    lock_t * head;
}lock_entry_t;

typedef struct lock_t{
    lock_t * prev;
    lock_t * next;
    lock_entry_t * sentinel;
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

/* APIs for lock table */
// Initialize any data structured required for implementing lock table
int init_lock_table(void);

// Allocate and append a new lock object to the lock list
// of the record having the key
lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode);

// Remove the lock_obj from the lock list
int lock_release(lock_t* lock_obj);


#endif //LOCK_MANAGER_H
