#include "buffer.h"
buffer_t * buffer;

int buffer_init_db(int num_buf){
    buffer = (buffer_t *)malloc(num_buf * sizeof(buffer_t));
    for(int i = 0; i < num_buf; i++){
        buffer->frame = make_page();
        buffer->table_id = -1;
        buffer->is_dirty = false;
        buffer->pin_count = 0;
        buffer->ref_bit = false;
    }
    return 0;
}

int buffer_open_table(const char * pathname){
    return file_open_table(pathname);
}

int buffer_close_table(int table_id){
    return file_close_table(table_id);
}

pagenum_t buffer_alloc_page(int table_id){

    return file_alloc_page(table_id);
}

void buffer_free_page(int table_id, pagenum_t pagenum){
    file_free_page(table_id, pagenum);
}

void buffer_read_page(int table_id, pagenum_t pagenum, page_t * dest){
    file_read_page(table_id, pagenum, dest);
}

void buffer_write_page(int table_id, pagenum_t pagenum, const page_t * src){
    file_write_page(table_id, pagenum, src);
}