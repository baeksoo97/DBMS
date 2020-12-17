#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unordered_map>
#include <set>
#include "file.h"
using namespace std;

#define LOG_FLAG_NORMAL 0
#define LOG_FLAG_REDO_CRASH 1
#define LOG_FLAG_UNDO_CRASH 2

#define LOG_SIZE_GENERAL 28
#define LOG_SIZE_UPDATE 288
#define LOG_SIZE_COMPENSATE 296

#define LOG_TYPE_BEGIN 0
#define LOG_TYPE_UPDATE 1
#define LOG_TYPE_COMMIT 2
#define LOG_TYPE_ROLLBACK 3
#define LOG_TYPE_COMPENSATE 4

#define LOG_POS_TYPE 24
#define LOG_FILE_START_POS 8
#define LOG_FILE_HEADER_SIZE 8

#define DATA_LENGTH 120
#define LOG_BUFFER_SIZE 1000 * 1000 // 1MB = 10^6

typedef int32_t logsize_t;
typedef int64_t lsn_t;
typedef int32_t trx_id_t;
typedef int32_t type_t;
typedef int32_t table_id_t;
typedef int32_t offset_t;
typedef int32_t data_length_t;
//typedef char image_t;

#pragma pack(push, 1)

typedef struct log_buffer_t{
    char buffer[LOG_BUFFER_SIZE];
    int offset = 0;
    pthread_mutex_t latch;
} log_buffer_t;

typedef struct general_log_t{
    logsize_t log_size;
    lsn_t lsn;
    lsn_t prev_lsn;
    trx_id_t trx_id;
    type_t type;
} general_log_t;

typedef general_log_t begin_log_t;
typedef general_log_t commit_log_t;
typedef general_log_t rollback_log_t;

typedef struct update_log_t{
    logsize_t log_size;
    lsn_t lsn;
    lsn_t prev_lsn;
    trx_id_t trx_id;
    type_t type;
    table_id_t table_id;
    pagenum_t pagenum;
    offset_t offset;
    data_length_t data_length;
    char old_img[DATA_LENGTH];
    char new_img[DATA_LENGTH];
} update_log_t;

typedef struct compensate_log_t{
    logsize_t log_size;
    lsn_t lsn;
    lsn_t prev_lsn;
    trx_id_t trx_id;
    type_t type;
    table_id_t table_id;
    pagenum_t pagenum;
    offset_t offset;
    data_length_t data_length;
    char old_img[DATA_LENGTH];
    char new_img[DATA_LENGTH];
    lsn_t next_undo_lsn;
} compensate_log_t;

#pragma pack(pop)

int init_log(int flag, int log_num, char * log_path, char * logmsg_path);
int start_recovery(int flag, int log_num, char * log_path, char * logmsg_path);
int do_recovery(logsize_t log_file_size, int flag, int log_num, char * log_path, char * logmsg_path);
offset_t do_analysis(unordered_map <trx_id_t, bool> &winner_trx_map);

void log_write_disk(void);
void log_write_buffer(logsize_t log_size, void * log);

lsn_t get_curr_lsn(logsize_t log_size);
lsn_t get_prev_lsn(trx_id_t trx_id, lsn_t lsn);

void log_write_general_record(type_t type, trx_id_t trx_id);
lsn_t log_write_update_record(trx_id_t trx_id, table_id_t table_id, pagenum_t pagenum, offset_t offset, char * old_img, char * new_img);
lsn_t log_write_compensate_record(trx_id_t trx_id, table_id_t table_id, pagenum_t pagenum, offset_t offset, char * old_img, char * new_img, lsn_t next_undo_lsn);

void log_read_record(void * dest);
void log_write_record(void * src);
#endif //LOG_MANAGER_H