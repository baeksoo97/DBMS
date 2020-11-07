#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__

#include <stdint.h>
#include <pthread.h>
#include <map>

typedef int table_id_t;
typedef int record_id_t;
typedef struct hash_table_t hash_table_t;
typedef struct lock_t lock_t;

/* APIs for lock table */
// Initialize any data structured required for implementing lock table
int init_lock_table();

// Allocate and append a new lock object to the lock list of the record having the key
lock_t* lock_acquire(int table_id, int64_t key);

// Remove the lock_obj from the lock list
int lock_release(lock_t* lock_obj);

#endif /* __LOCK_TABLE_H__ */
