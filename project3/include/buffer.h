#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "file.h"
#include <unordered_map>
typedef int framenum_t;
typedef unordered_map<pagenum_t, framenum_t> frame_map_t;

typedef struct buffer_header{
    // value -1 means buffer doesn't have key pagenum
    vector <frame_map_t > frame_map;
    int num_buf;
} buffer_header_t;

typedef struct buffer_t{
    page_t * frame;
    int table_id;
    pagenum_t pagenum;
    bool is_dirty;
    int pin_count;
    buffer_t * prev;
    buffer_t * next;
} buffer_t;


static buffer_t * buffer = NULL;
static buffer_header_t buffer_header;

int buffer_init_db(int num_buf);

void buffer_close_db(void);

int buffer_open_table(const char * pathname);

int buffer_close_table(int table_id);

int buffer_is_opened(int table_id);

pagenum_t buffer_alloc_page(int table_id);

void buffer_free_page(int table_id, pagenum_t pagenum);

framenum_t buffer_find_frame(int table_id, pagenum_t pagenum);

void buffer_read_page(int table_id, pagenum_t pagenum, page_t * dest);

void buffer_write_page(int table_id, pagenum_t pagenum, const page_t * src);

#endif //__BUFFER_H__
