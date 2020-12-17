#include "log_manager.h"
int log_fd;
FILE * logmsg_fp;

lsn_t global_lsn = 0;

unordered_map <trx_id_t, lsn_t> prev_lsn_map;

pthread_mutex_t log_manager_latch;

log_buffer_t read_buffer;
log_buffer_t write_buffer;

int init_log(int flag, int log_num, char * log_path, char * logmsg_path) {
    pthread_mutex_init(&log_manager_latch, NULL);
    pthread_mutex_init(&read_buffer.latch, NULL);
    pthread_mutex_init(&write_buffer.latch, NULL);

//    if (access(log_path, F_OK) || access(logmsg_path, F_OK)){
//        // files don't exist
//        log_fd = open(log_path, O_RDWR, 0644); // open, read, write
//        if (log_fd == -1) {
//            perror("ERROR INIT LOG : cannot open log file\n");
//            return -1;
//        }
//    }

    if ((logmsg_fp = fopen(logmsg_path,"w+")) == NULL){
        // open, read, write
        return -1;
    }

    return 0;
}

int start_recovery(int flag, int log_num, char * log_path, char * logmsg_path){
    logsize_t log_file_size;

    // nothing to recover
    if (access(log_path, F_OK)){
        log_fd = open(log_path, O_RDWR | O_CREAT, 0644);
        write(log_fd, &global_lsn, sizeof(lsn_t));
        fclose(logmsg_fp);
        return 0;
    }

    log_fd = open(log_path, O_RDWR, 0644);

    // nothing to recover
    // the first of file is always LSN
    log_file_size = lseek(log_fd, 0, SEEK_END);
    if (log_file_size == sizeof(lsn_t)){
        return 0;
    }

    do_recovery(log_file_size, flag, log_num, log_path, logmsg_path);
    return 0;
}

int do_recovery(logsize_t log_file_size, int flag, int log_num, char * log_path, char * logmsg_path){
    unordered_map <trx_id_t, bool> winner_trx_map;
    unordered_map <trx_id_t, bool>::iterator it;
    int read_cnt = 0;
    offset_t pos = LOG_FILE_START_POS;

    /* ---------------------- ANALYSIS ---------------------- */
    fprintf(logmsg_fp, "[ANALYSIS] Analysis pass start\n");

    logsize_t remain_file_size = log_file_size;

    while (remain_file_size){
        read_cnt = remain_file_size >= LOG_BUFFER_SIZE ? LOG_BUFFER_SIZE : remain_file_size;

        if (pread(log_fd, read_buffer.buffer, read_cnt, pos) == -1){
            printf("ERROR \n");
        }

        read_cnt = do_analysis(winner_trx_map);
        remain_file_size -= read_cnt;
        pos += read_cnt;
    }

    fprintf(logmsg_fp, "[ANALYSIS] Analysis success. ");

    fprintf(logmsg_fp, "Winner:");

    for (it = winner_trx_map.begin(); it != winner_trx_map.end(); it++){
        if (it->second)
            fprintf(logmsg_fp, " %d", it->first);
    }
    fprintf(logmsg_fp, ", Loser:");
    for (it = winner_trx_map.begin(); it != winner_trx_map.end(); it++){
        if (!it->second)
            fprintf(logmsg_fp, " %d", it->first);
    }
    fprintf(logmsg_fp, "\n");

    /* ---------------------- REDO ---------------------- */
    fprintf(logmsg_fp, "[REDO] REDO pass start\n");


    fprintf(logmsg_fp, "[REDO] REDO pass end\n");



    /* ---------------------- UNDO ---------------------- */

    fprintf(logmsg_fp, "[UNDO] UNDO pass start\n");


    fprintf(logmsg_fp, "[UNDO] UNDO pass end\n");


}

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

        printf("curr pos %d / log size %d\n", curr_pos, log_size);
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

int do_redo(){

}

int do_undo(){

}


void log_flush_file(void){

}

void log_print_file(void){

//    fprintf(logmsg_fp, "[ANALYSIS] Analysis pass start\n");
//    fprintf(logmsg_fp, "[ANALYSIS] Analysis success. Winner : , Loser : \n");
    fprintf(logmsg_fp, "[REDO(UNDO)] REDO(UNDO) pass start\n");
//    fprintf(logmsg_fp, "LSN %lu[BEGIN] Transaction id %d\n", lsn, trx_id);
//    fprintf(logmsg_fp, "LSN %lu[UPDATE] Transaction id %d redo(undo) apply\n", lsn, trx_id);
//    fprintf(logmsg_fp, "LSN %lu[COMMIT] Transaction id %d\n", lsn, trx_id);
//    fprintf(logmsg_fp, "LSN %lu[ROLLBACK] Transaction id %d\n", lsn, trx_id);
//    fprintf(logmsg_fp, "LSN %lu[CLR] next undo lsn %lu\n", lsn, next_undo_lsn);
//    fprintf(logmsg_fp, "LSN %lu[CONSIDER-REDO] Transaction id %d\n", lsn, trx_id);
    fprintf(logmsg_fp, "[REDO(UNDO)] REDO(UNDO) pass end\n");
}

void log_write_disk(){
    offset_t start_pos;

    start_pos = lseek(log_fd, 0, SEEK_END);

    if (pwrite(log_fd, &global_lsn, LOG_FILE_HEADER_SIZE, 0) == -1){
        printf("ERROR LOG_WRITE_DISK : cannot write global LSN into disk\n");
    }

    if (pwrite(log_fd, &write_buffer.buffer, LOG_BUFFER_SIZE, start_pos) == -1){
        printf("ERROR LOG_WRITE_DISK : cannot write buffer into disk\n");
    }

    write_buffer.offset = 0;
}

void log_write_buffer(logsize_t log_size, void * log){
    pthread_mutex_lock(&write_buffer.latch);

    if (write_buffer.offset + log_size > LOG_BUFFER_SIZE){
        log_write_disk();
    }

    memcpy(&write_buffer.buffer[write_buffer.offset], log, log_size);
    write_buffer.offset += log_size;

    pthread_mutex_unlock(&write_buffer.latch);
}

lsn_t get_curr_lsn(logsize_t log_size){
    lsn_t curr_lsn;

    curr_lsn = global_lsn;
    global_lsn += log_size;

    return curr_lsn;
}

lsn_t get_prev_lsn(trx_id_t trx_id, lsn_t lsn){
    lsn_t prev_lsn;

    if (prev_lsn_map.count(trx_id)){
        prev_lsn = prev_lsn_map[trx_id];
    }
    else{
        prev_lsn = -1;
    }
    prev_lsn_map[trx_id] = lsn;

    return prev_lsn;
}

void log_write_general_record(type_t type, trx_id_t trx_id){
    pthread_mutex_lock(&log_manager_latch);
    general_log_t general_log;

    general_log.log_size = LOG_SIZE_GENERAL;
    general_log.lsn = get_curr_lsn(LOG_SIZE_GENERAL);
    general_log.prev_lsn = get_prev_lsn(trx_id, general_log.lsn);
    general_log.trx_id = trx_id;
    general_log.type = type;

    global_lsn = general_log.lsn;

//    ssize_t write_size = write(log_fd, src, sizeof(src));
    log_write_buffer(general_log.log_size, &general_log);

    pthread_mutex_unlock(&log_manager_latch);
}

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

// Read an on-disk page into the in-memory page structure(dest)
void log_read_record(void * dest){
    lseek(log_fd, 0, SEEK_END);
    ssize_t read_size = read(log_fd, dest, sizeof(dest));

    if (read_size == -1)
        perror("ERROR LOG_READ_RECORD\n");
}

// Write an in-memory page(src) to the on-disk page
void log_write_record(void * src){
    lseek(log_fd, 0, SEEK_END);
    ssize_t write_size = write(log_fd, src, sizeof(src));
    fsync(log_fd);

    if (write_size == -1)
        perror("ERROR LOG_WRITE_RECORD\n");
}

