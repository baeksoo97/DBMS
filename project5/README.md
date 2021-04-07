# Lock Manager

## Structures

### lock_entry_t

```cpp
typedef struct lock_entry_t{
    int table_id;
    int64_t key;
    lock_t * head;  // the first lock of the lock entry
    lock_t * tail;  // the last lock of the lock entry
}lock_entry_t;
```

### lock_t

```cpp
typedef struct lock_t{
    lock_t * prev;           // the previous lock in the lock entry
    lock_t * next;           // the next lock in the lock entry
    lock_t * trx_next_lock;  // the next lock in same transaction
    lock_t * trx_prev_lock;  // the previous lock in same transaction
    lock_entry_t * sentinel; // the lock entry where the lock is
    pthread_cond_t cond;     // the condition variable for locking record
    int lock_mode;           // SHARED = 0, EXCLUSIVE = 1
    int owner_trx_id;        // the id of transaction where the lock is
    bool is_waiting;         // waiting = 1, working = 0
}lock_t;
```

### lock_table

```cpp
typedef pair<int, int64_t> lock_key_t;  // pair<table_id, key>
unordered_map <lock_key_t, lock_entry_t> lock_table;
```

## Functions

### `lock_acquire()`

- This is a function for acquiring lock for record. It allocates and append a new lock object to the lock list of the record having the key.
    - **Case 1 ) If there is a predecessor's lock object in the lock list,**
        - Sleep until the predecessor to release its lock if there is no locks having same `trx_id` with the new lock object. Or, if there is a lock which is in the same transaction with the new lock object, change the lock mode that has been already acquired (if it should be changed) and return it.
        - **Case 1 - 1 )  If there is a working lock which is in same transaction,**
            - **Case 1 - 1 - 1 ) working lock = `LOCK_SHARED`**
                - new lock = `LOCK_SHARED` : just return the working lock, because there's no need to acquire lock.
                - new_lock = `LOCK_EXCLUSIVE`
                    - If my transaction's working lock is the only one which is working, upgrade the mode to `LOCK_EXCLUSIVE`.
                    - Or not, acquire new lock and append it to the lock list and check deadlock. If deadlock is detected, return nullptr. Otherwise, wait for the lock by calling `pthread_cond_wait`
            - **Case 1 - 1 - 2 ) working lock = `LOCK_EXCLUSIVE`**
                - return this working lock
        - **Case 1 - 2 )   If there is no working lock which is in same transaction,** create new lock and append it to the lock list and the transaction list.
            - **Case 1 - 2 - 1 ) If the previous of new lock is working,**
                - **Case 1 - 2 - 1 - 1 ) the previous working lock = `LOCK_SHARED`**
                    - new lock = `LOCK_SHARED` : do nothing
                    - new lock = `LOCK_EXCLUSIVE` : Check deadlock
                        - If there it is, return `nullptr`.
                        - Or not, wait for the lock by using `pthread_cond_wait()`.
                - **Case 1 - 2 - 1 - 2 ) the previous working lock = `LOCK_EXCLUSIVE`**
                    - new lock = `LOCK_EXCLUSIVE` : Check deadlock
                        - If there it is, return `nullptr`.
                        - Or not, wait for the lock by using `pthread_cond_wait()`.
            - **Case 1 - 2 - 2 ) If the previous of new lock is waiting,**
                - Check deadlock.
                    - If there it is, return `nullptr`.
                    - Or not, wait for the lock by using `pthread_cond_wait()`.
- **Case 2) If there is no lock in the lock list,**
    - create new lock and append it to the lock list and the transaction list.

## `lock_release()`

- This is a function for removing lock object from the lock list. It sends signal only when the lock object is the head of lock list and the next of lock object is waiting for the lock.
    - **Case 1 ) If the lock object is the head of lock list,**
        - **Case 1 - 1 ) If there is a successor's lock waiting for the thread,**
            - **Case 1 - 1 - 1 ) If the next of lock object is waiting for the lock,**
                - the next lock object = `LOCK_SHARED`
                    - the current lock object = `LOCK_SHARED` : do nothing
                    - the current lock object = `LOCK_EXCLUSIVE` : wake up all the next shared mode of lock object
                - the next lock object = `LOCK_EXCLUSIVE`
                    - wake up the next lock object
        - **Case 1 - 2 ) If there is no more lock in the lock list,**
            - erase the lock entry containing the lock from the lock table
    - **Case 2 ) If the lock object is not the head of lock list,**
        - **Case 2 - 1 ) If the lock object is not the tail of lock list,** update the locks around it and wake up all next shared mode locks.
        - **Case 2 - 2 ) If the lock object is the tail of lock list,** update the tail and it's previous one.
    - Special Case ) After removing the lock from the lock list, if there are working lock and waiting lock which have same transaction id, wake up the waiting lock.

## `check_deadlock()`

- This is a function for checking whether there is deadlock or not. It calls `get_wait_for_graph()` in transaction layer. It detects a cycle from  the wait for list.
    - return True (It has deadlock after append the lock to the lock list
    - return False (It doesn't have deadlock after appending the lock to the lock list)

# Transaction Layer

## Structures

### trx_entry_t

```cpp
typedef struct trx_entry_t{
    int trx_id;
    lock_t * head;
    lock_t * tail;
    unordered_map <log_key_t, log_t, hash_pair> log_map; 
		  // [ (table_id, key) : log_t ] map
			// [ (1, 10) : {1, 10, "old_value10"}, (1, 20) : {1, 20, "old_value20"} ...]
}trx_entry_t;
```

### log_t

This is a structure for storing the old value of db_update().

```cpp
typedef struct log_t{
    int table_id;
    int64_t key;
    char old_value[120];
}log_t;
```

## Functions

### `trx_begin()`

- This is a function for allocating a transaction structure and initialize it. It returns a unique transaction if success, otherwise return 0.
- The transaction id should be unique for each transaction, so the transaction id should hold a mutex.

### `trx_commit()`

- This is a function for cleaning up the transaction with given transaction id and its related information that has been used in lock manager.
    - This function calls lock_release for strict two phase lock like below.

        ```cpp
        lock = trx_manager[trx_id].head;

        while(lock != NULL){
            tmp_lock = lock;
            lock = lock->trx_next_lock;
            lock_release(tmp_lock); // strict two phase lock
        }

        trx_manager.erase(trx_id);
        ```

### `trx_abort()`

- This is a function for releasing the locks that are held on this transaction and rollback all operation in transaction.
- At first, rollback operations which are already completed by using `undo` function and erase `undo_map` .
- And then, release all locks that are held on the transaction and erase this transaction from transaction table.

### `get_wait_for_graph()`

- This is a function for acquiring wait for graph for detecting cycle.

# Index Layer

## trx_find

- This is a function for reading a value in the table with matching key for the transaction having `trx_id`.
    - return 0(SUCCESS) : operation is successfully done and the transaction can continue the next operation
    - return -1(FAILED) : operation is failed (deadlock detected).
    - It finds the page in the buffer, and after finding the page, it tries to acquire lock for the  record (table_id, key).
    - If deadlock is detected while acquiring lock, it should return -1.
    - After acquiring lock, It should read the page one more time from buffer.

## trx_update

- This is a function for finding the matching key and modifying the value.
    - return 0(SUCCESS) : operation is successfully done and the transaction can continue the next operation
    - return -1(FAILED) : operation is failed (deadlock detected).
    - It finds the page in the buffer, and after finding the page, it tries to acquire lock for the  record (table_id, key).
    - If deadlock is detected while acquiring lock, it should return -1.
    - After acquiring lock, It should update the record with new value.
    - For rollback operation, the old value before updating should be stored in `undo_map` of its transaction by using function `trx_write_log`.

## undo

- This is a function for rollback of previous operations.
- If deadlock is detected after trying to acquire lock, the transaction which includes the lock should be aborted. To rollback previous operations of the transaction, the old values which were used before updating the records should be stored in `undo_map` of the transaction structure.
