#include "buffer.h"

int buffer_open_table(const char * pathname){
    return file_open_table(pathname);
}

int buffer_init_db(int num_buf){
    return 0;
}

int buffer_close_table(int table_id){
    return file_close_table(table_id);
}