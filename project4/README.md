# Lock Table

## Lock Table APIs

### Data Structures

#### Hash Entry Structure

```cpp
typedef struct hash_entry_t{
    int table_id;      // the table id of key
    int64_t record_id; // the record id of key
    lock_t * tail;     // the end of lock list
    lock_t * head;     // the start of lock list
}hash_entry_t;
```

#### Lock Structure

```cpp
typedef struct lock_t {
    lock_t * prev;           // the next lock object in the list
    lock_t * next;           // the previous lock object in the list
    hash_entry_t * sentinel; // the lock list to which the lock object belongs
    pthread_cond_t cond;     // conditional lock that the lock object jas
}lock_t;
```

#### Hash Table

- I implement the hash table that contains lock list using by `unordered_map`.

```cpp
typedef struct hash_pair {
    template <class T1, class T2>
    size_t operator() (const pair<T1, T2> & p) const{
        return hash<T1>()(p.first) ^ hash<T2>()(p.second);
    }
}hash_pair;

typedef pair<int, int64_t> hash_key_t;
unordered_map <hash_key_t, hash_entry_t, hash_pair> hash_table;
```

### Functions

#### `init_lock_table()`

- This is a function for initializing lock table latch required for implementing lock table.

    ```cpp
    int init_lock_table()
    {
        pthread_mutex_init(&lock_table_latch, NULL);

    	return 0;
    }
    ```

#### `lock_acquire()`

- This is a function for allocating and appending a new lock object to the lock list of the record having the key.
- There are two cases for allocating and appending a new lock object.
    - Case 1 : If there is a predecessor's lock object in the lock list
        - In this case, allocate the new lock object with calling `pthread_cond_init` and append it  to the end of the lock list with updating the tail of the lock list and hash table. Then, make it sleep until the predecessor to release its lock by calling `pthread_cond_wait`.
    - Case 2 : If there is no predecessor's lock object int he lock list.
        - It means that the new lock object can acquire the lock so that just append it to the lock list and update hash table.
- `pthread_mutex_lock` and `pthread_mutex_unlock` should be called in the start and the last of function. It can project the acquire function as a critical section.

    ```cpp
    lock_t* lock_acquire(int table_id, int64_t key)
    {
        unordered_map<hash_key_t, hash_entry_t>::iterator it;
        hash_entry_t hash_entry;
        lock_t * lock, * tail;

        pthread_mutex_lock(&lock_table_latch);

        it = hash_table.find(make_pair(table_id, key));
        lock = (lock_t *)malloc(sizeof(lock_t));
        if (lock == NULL){
            pthread_mutex_unlock(&lock_table_latch);
            return nullptr;
        }

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

            pthread_cond_wait(&(lock->cond), &lock_table_latch);

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

        pthread_mutex_unlock(&lock_table_latch);
        return lock;
    }
    ```

#### `lock_release()`

- This is a function for removing `lock_obj` from the lock list.
- There are two cases for removing the `lock_obj` form the lock list.
    - Case 1 : If there is a successor's lock waiting for the thread releasing the lock.
        - In this case, update the lock list and the lock_obj and then wake up the successor by calling `pthread_cond_signal` with the conditional variable of the successor.
    - Case 2 :  If there is no successor's lock waiting for the thread releasing the lock.
        - In this case, just update the lock list and remove the lock list from hash table.
- `pthread_mutex_lock` and `pthread_mutex_unlock` should be called in the start and the last of function. It can project the release function as a critical section.

    ```cpp
    int lock_release(lock_t* lock_obj)
    {
        pthread_mutex_lock(&lock_table_latch);

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

        pthread_mutex_unlock(&lock_table_latch);
    	  return 0;
    }
    ```
