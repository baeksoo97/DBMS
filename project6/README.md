# Log Manager

## Log Struct

### BEGIN, COMMIT, ROLLBACK  Log Layout

```cpp
typedef int32_t logsize_t;      // 4byte
typedef int64_t lsn_t;          // 8byte
typedef int32_t trx_id_t;       // 4byte
typedef int32_t type_t;         // 4byte
typedef int32_t table_id_t;     // 4byte
typedef int32_t offset_t;       // 4byte
typedef int32_t data_length_t;  // 4byte
```

```cpp
typedef struct general_log_t{
    logsize_t log_size;      // 4byte
    lsn_t lsn;               // 8byte
    lsn_t prev_lsn;          // 8byte
    trx_id_t trx_id;         // 4byte
    type_t type;             // 4byte
} general_log_t;             // 28byte : total

typedef general_log_t begin_log_t;
typedef general_log_t commit_log_t;
typedef general_log_t rollback_log_t;
```

### UPDATE Log Layout

```cpp
typedef struct update_log_t{
    logsize_t log_size;        // 4byte
    lsn_t lsn;                 // 8byte
    lsn_t prev_lsn;            // 8byte
    trx_id_t trx_id;           // 4byte
    type_t type;               // 4byte
    table_id_t table_id;       // 4byte
    pagenum_t pagenum;         // 8byte
    offset_t offset;           // 4byte
    data_length_t data_length; // 4byte
    char old_img[DATA_LENGTH]; // 120byte 
    char new_img[DATA_LENGTH]; // 120byte
} update_log_t;                // 288byte : total
```

### COMPENSATE Log Layout

```cpp
typedef struct compensate_log_t{
    logsize_t log_size;        // 4byte
    lsn_t lsn;                 // 8byte
    lsn_t prev_lsn;            // 8byte
    trx_id_t trx_id;           // 4byte
    type_t type;               // 4byte
    table_id_t table_id;       // 4byte
    pagenum_t pagenum;         // 8byte
    offset_t offset;           // 4byte
    data_length_t data_length; // 4byte
    char old_img[DATA_LENGTH]; // 120byte 
    char new_img[DATA_LENGTH]; // 120byte
    lsn_t next_undo_lsn;       // 8byte
} compensates_log_t;           // 296byte : total
```

# Recovery

## Log

Recovery를 위한 Log 파일을 생성하기 위해 Transaction의 operation들이 수행되는 과정에서 Begin, Commit, Rollback, Update, Compensate Log가 Log 파일에 기록된다. 각각의 Log들이 작성되는 시점과 과정은 아래와 같다.

### Begin Log

Begin Log는 Transaction의 수행을 위해서 설정되고 시작되는 시점인 `trx_begin()` 함수에서 생성된다. 이 때 아래와 같은 `log_write_general_record()` 함수를 `LOG_TYPE_BEGIN` 을 전달인자와 함께 호출한다. Begin Log를 생성한 후에 이를 Log Buffer에 작성해야 한다. 이 때 반드시 `log_manager_latch` 락을 잡고 있는 상태에서 Begin Log를 생성하고 Buffer에 작성해야 한다. 

```cpp
void log_write_general_record(type_t type, trx_id_t trx_id){
    pthread_mutex_lock(&log_manager_latch);
    general_log_t general_log;

    general_log.log_size = LOG_SIZE_GENERAL;
    general_log.lsn = get_curr_lsn(LOG_SIZE_GENERAL);
    general_log.prev_lsn = get_prev_lsn(trx_id, general_log.lsn);
    general_log.trx_id = trx_id;
    general_log.type = type;

    global_lsn = general_log.lsn;

    log_write_buffer(general_log.log_size, &general_log);

    pthread_mutex_unlock(&log_manager_latch);
}
```

### Commit Log

Commit Log는 Transaction이 완료되는 시점인 `trx_commit()`함수에서 생성된다. Begin Log와 같이  `log_write_general_record()` 함수를 호출하여 같은 과정으로 수행된다.

### Rollback Log

Rollback Log는 Transaction이 abort와 함께 완료되는 시점인 `trx_abort()` 함수에서 생성된다. Begin Log와 같이 `log_write_general_record()` 함수를 호출하여 같은 과정으로 수행된다.

### Update Log

Update Log는 Transaction중 Update Operation으로 인해 record의 값이 변경되었을 경우 Log File에 작성되어야 한다. `trx_update()` 함수 내에서 record의 값이 변경되기 직전에 아래와 같은 `log_write_update_record()` 함수를 호출하여 Update Log를 생성하고 이를 Log Buffer에 작성한다.

```cpp
lsn_t log_write_update_record(trx_id_t trx_id, table_id_t table_id, pagenum_t pagenum, offset_t offset, char * old_img, char * new_img){
    pthread_mutex_lock(&log_manager_latch);

    update_log_t update_log;

    update_log.log_size = LOG_SIZE_UPDATE;
    update_log.lsn = get_curr_lsn(LOG_SIZE_UPDATE);
    update_log.prev_lsn = get_prev_lsn(trx_id, update_log.lsn);
    update_log.trx_id = trx_id;
    update_log.type = LOG_TYPE_UPDATE;
    update_log.table_id = table_id;
    update_log.pagenum = pagenum;
    update_log.offset = offset;
    update_log.data_length = DATA_LENGTH;
    strcpy(update_log.old_img, old_img);
    strcpy(update_log.new_img, new_img);

    log_write_buffer(update_log.log_size, &update_log);

    pthread_mutex_unlock(&log_manager_latch);

}
```

### Compensate Log

Compensate Log는 Transaction이 abort되어 operation들을 undo했을 시 생성된다. `trx_abort` 함수 내에서 undo를 하기 직전에 아래와 같은 `log_write_compensate_record()` 함수를 호출하여 Compensate Log를 생성하고 이를 Log Buffer에 작성한다.

```cpp
lsn_t log_write_compensate_record(trx_id_t trx_id, table_id_t table_id, pagenum_t pagenum, offset_t offset, char * old_img, char * new_img, lsn_t next_undo_lsn){
    pthread_mutex_lock(&log_manager_latch);

    compensate_log_t compensate_log;

    compensate_log.log_size = LOG_SIZE_COMPENSATE;
    compensate_log.lsn = get_curr_lsn(LOG_SIZE_COMPENSATE);
    compensate_log.prev_lsn = get_prev_lsn(trx_id, compensate_log.lsn);
    compensate_log.trx_id = trx_id;
    compensate_log.type = LOG_TYPE_COMPENSATE;
    compensate_log.table_id = table_id;
    compensate_log.pagenum = pagenum;
    compensate_log.offset = offset;
    compensate_log.data_length = DATA_LENGTH;
    strcpy(compensate_log.old_img, old_img);
    strcpy(compensate_log.new_img, new_img);
    compensate_log.next_undo_lsn = next_undo_lsn;

    log_write_buffer(compensate_log.log_size, &compensate_log);

    pthread_mutex_unlock(&log_manager_latch);
}
```

만약 Log Buffer가 꽉찼을 경우, Log File에 Log Buffer의 Log들을 순차적으로 작성해준다. 

## Analysis Pass

 Analysis Pass는 어떤 Log부터 Redo pass 를 시작할지를 결정한다. 즉 Winner Transaction과 Loser Transaction을 판별해야 하는데, 이는 Log 파일에 저장되어있는 Log들을 보며 **1)BEGIN과 COMMIT 로그가 동시에 존재하는 Transaction, 2)BEGIN과 ROLLBACK 로그가 동시에 존재하는 Transaction**을 찾아낸다. 이는 Winner에 해당되고, 그 외의 Transaction들은 Loser로 분류한다.

 WAL 프로토콜은 BEGIN이 존재하지 않으면  COMMIT이나 ROLLBACK 로그가 존재하지 않음을 보장하므로, BEGIN 로그가 없이 완료될 Winner에 대해서 생각하지 않는다.

```cpp
offset_t do_analysis(unordered_map <trx_id_t, bool> &winner_trx_map){
    offset_t curr_pos;
    logsize_t log_size;
    general_log_t general_log;

    curr_pos = 0;

    while (true){
        if (curr_pos + sizeof(logsize_t) > LOG_BUFFER_SIZE){
            return curr_pos;
        }

        memcpy(&log_size, &read_buffer.buffer[curr_pos], sizeof(logsize_t));
        if (curr_pos + log_size > LOG_BUFFER_SIZE){
            return curr_pos;
        }

        //printf("curr pos %d / log size %d\n", curr_pos, log_size);
        if (log_size == LOG_SIZE_GENERAL){
            memcpy(&general_log, &read_buffer.buffer[curr_pos], LOG_SIZE_GENERAL);

            if (general_log.type == LOG_TYPE_BEGIN){
                winner_trx_map[general_log.trx_id] = false;
            }
            else if (general_log.type == LOG_TYPE_COMMIT
                    || general_log.type == LOG_TYPE_ROLLBACK){
                winner_trx_map[general_log.trx_id] = true;
            }
        }

        curr_pos += log_size;
    }

    return curr_pos;
}
```

## Redo Pass

Redo Pass에서 ARIES는 모든 Transaction의 업데이트를 다시 적용한다. 즉, Redo Pass는 Durability를 충족시키기 위해 모든 커밋된 Transaction의 결과를 다시 적용시키는 것이다.

Redo Pass의 과정은 아래와 같이 두 가지로 나뉘어진다. 

- Case 1 : Redo -  log LSN > page LSN
    - 로그의 내용이 page의 내용보다 더 최신일 때는, 페이지에 로그의 정보를 반영시킨다.
- Case 2 : Consider Redo - log LSN ≤ page LSN
    - page의 내용이 로그의 내용보다 최신일 때는, 로그의 정보를 페이지에 반영시키지 않는다.

## Undo Pass

Undo는 Atomicity를 충족시키기 위해 존재한다. Loser Transaction들(Commit되지 않은 Transaction)의 update operation들을 Undo Pass로 취소시킨다. 

Undo Pass는 Analysis와 Redo와 달리 가장 뒤에 있는 로그부터 시작하여 끝에서 앞으로 로그를 스캔한다. Analysis Pass에서 식별하였던 Loser Transaction에 대한 정보와 함께 해당 Transaction 로그들을 undo 시킨다. 

# Simple Test Result

### init_db test

![Untitled](uploads/d8568c0d820107093cff70f58d7b542e/Untitled.png)

### single_thread_test

![Untitled2](uploads/ea45a68f54de1a71ca485cc1219195e2/Untitled2.png)
