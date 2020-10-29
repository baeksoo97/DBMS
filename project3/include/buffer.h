#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "file.h"

typedef struct buffer_t{
    page_t * frame;
    int table_id;
    pagenum_t pagenum;
    bool is_dirty;
    int pin_count;
    bool ref_bit;
} buffer_t;

extern buffer_t * buffer;

int buffer_init_db(int num_buf);

int buffer_open_table(const char * pathname);

int buffer_close_table(int table_id);

int buffer_is_opened(int table_id);

pagenum_t buffer_alloc_page(int table_id);

void buffer_free_page(int table_id, pagenum_t pagenum);

void buffer_read_page(int table_id, pagenum_t pagenum, page_t * dest);

void buffer_write_page(int table_id, pagenum_t pagenum, const page_t * src);

#endif //__BUFFER_H__
