#include "transaction.h"

/*
    Initialize any data structured required for implementing lock table
*/
int init_lock_table()
{
    pthread_mutex_init(&lock_manager_latch, NULL);

    return 0;
}

/*
    Allocate and append a new lock object to the lock list
    of the record having the key
*/
lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode)
{
    unordered_map<hash_key_t, hash_entry_t, hash_pair>::iterator it;
    hash_entry_t hash_entry;
    lock_t * lock, * tail;

    pthread_mutex_lock(&lock_manager_latch);

    it = hash_table.find(make_pair(table_id, key));
    lock = (lock_t *)malloc(sizeof(lock_t));
    if (lock == NULL){
        pthread_mutex_unlock(&lock_manager_latch);
        return nullptr;
    }
//    printf("lock acquire %d, %d\n", table_id, key);

    // if there is a predecessor's lock object in the lock list,
    // sleep until the predecessor to release its lock.
    if (it != hash_table.end()){
        tail = it->second.tail;

        tail->next = lock;
        lock->prev = tail;
        lock->next = NULL;
        lock->sentinel = &it->second;
        pthread_cond_init(&(lock->cond), NULL);

        hash_table[make_pair(table_id, key)].tail = lock;

        pthread_cond_wait(&(lock->cond), &lock_manager_latch);

    }
    // if there is no predecessor's lock object,
    // return the address of the new lock object.
    else{
        hash_entry.table_id = table_id;
        hash_entry.record_id = key;
        hash_entry.tail = lock;
        hash_entry.head = lock;

        hash_table[make_pair(table_id, key)] = hash_entry;

        lock->prev = NULL;
        lock->next = NULL;
        lock->sentinel = &hash_table[make_pair(table_id, key)];
        pthread_cond_init(&(lock->cond), NULL);
    }

    pthread_mutex_unlock(&lock_manager_latch);
    return lock;
}

/*
    Remove the lock_obj from the lock list
*/
int lock_release(lock_t* lock_obj)
{
    pthread_mutex_lock(&lock_manager_latch);

    // if there is a successor's lock waiting for the thread
    // releasing the lock, wake up the successor.
    if (lock_obj->next != NULL){
        lock_obj->sentinel->head = lock_obj->next;
        lock_obj->next->prev = NULL;
        pthread_cond_signal(&(lock_obj->next->cond));

    }
    else{
        hash_entry_t * hash_entry = lock_obj->sentinel;
        hash_entry->head = NULL;
        hash_entry->tail = NULL;
        hash_key_t hash_key = make_pair(hash_entry->table_id, hash_entry->record_id);

        hash_table.erase(hash_table.find(hash_key));
    }

    free(lock_obj);

    pthread_mutex_unlock(&lock_manager_latch);
    return 0;
}
