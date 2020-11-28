#include "lock_manager.h"

static unordered_map <lock_key_t, lock_entry_t, hash_pair> lock_table;
static pthread_mutex_t lock_manager_latch;

/*
    Initialize any data structured required for implementing lock table
*/
int init_lock_table(void){
    int ret = pthread_mutex_init(&lock_manager_latch, NULL);
    if (ret != 0) return -1;
    else return 0;
}

/*
    Allocate and append a new lock object to the lock list
    of the record having the key
*/
lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode){
    pthread_mutex_lock(&lock_manager_latch);

    unordered_map<lock_key_t, lock_entry_t, hash_pair>::iterator it;
    lock_entry_t lock_entry;
    lock_t * lock, * tail;

    lock = (lock_t *)malloc(sizeof(lock_t));
    if (lock == NULL){
        pthread_mutex_unlock(&lock_manager_latch);
        return nullptr;
    }
//    printf("lock acquire %d, %d\n", table_id, key);

    it = lock_table.find(make_pair(table_id, key));

    // if there is a predecessor's lock object in the lock list,
    // sleep until the predecessor to release its lock.
    if (it != lock_table.end()){
        tail = it->second.tail;

        tail->next = lock;
        lock->prev = tail;
        lock->next = NULL;
        lock->sentinel = &it->second;
        pthread_cond_init(&(lock->cond), NULL);

        lock_table[make_pair(table_id, key)].tail = lock;

        pthread_cond_wait(&(lock->cond), &lock_manager_latch);

    }
        // if there is no predecessor's lock object,
        // return the address of the new lock object.
    else{
        lock_entry.table_id = table_id;
        lock_entry.record_id = key;
        lock_entry.tail = lock;
        lock_entry.head = lock;

        lock_table[make_pair(table_id, key)] = lock_entry;

        lock->prev = NULL;
        lock->next = NULL;
        lock->sentinel = &lock_table[make_pair(table_id, key)];
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
        lock_entry_t * lock_entry = lock_obj->sentinel;
        lock_entry->head = NULL;
        lock_entry->tail = NULL;
        lock_key_t hash_key = make_pair(lock_entry->table_id, lock_entry->record_id);

        lock_table.erase(lock_table.find(hash_key));
    }

    free(lock_obj);

    pthread_mutex_unlock(&lock_manager_latch);
    return 0;
}