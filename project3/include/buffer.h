#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "file.h"
#include <unordered_map>
#include <queue>

#define NONE -1
typedef int framenum_t;
typedef unordered_map<pagenum_t, framenum_t> frame_map_t;

typedef struct buffer_t{
    page_t * frame;
    int table_id;
    pagenum_t pagenum;
    bool is_dirty;
    int pin_cnt;
    framenum_t prev;
    framenum_t next;
} buffer_t;

typedef struct buffer_header_t{
    // value -1 means buffer doesn't have key pagenum
    vector <frame_map_t > frame_map;
    framenum_t buf_capacity;
    framenum_t buf_size;
    framenum_t head;
    framenum_t tail;
    bool is_open;
} buffer_header_t;

static buffer_t * buffer = NULL;
static buffer_header_t buffer_header;

int buffer_init_db(int num_buf);

void buffer_close_db(void);

int buffer_open_table(const char * pathname);

void buffer_flush_frame(framenum_t frame_idx);

void buffer_flush_table(int table_id);

int buffer_close_table(int table_id);

int buffer_is_opened(int table_id);

page_t * buffer_read_header(int table_id);

pagenum_t buffer_alloc_page(int table_id);

void buffer_free_page(int table_id, pagenum_t pagenum);

void buffer_lru_update(framenum_t frame_idx);

framenum_t buffer_lru_frame(void);

int buffer_init_frame(int table_id, framenum_t frame_idx, pagenum_t pagenum, page_t * dest);

void buffer_alloc_frame(int table_id, pagenum_t pagenum, page_t * dest);

framenum_t buffer_find_frame(int table_id, pagenum_t pagenum);

void buffer_read_page(int table_id, pagenum_t pagenum, page_t * dest);

void buffer_write_page(int table_id, pagenum_t pagenum, page_t * src);

void buffer_unpin_frame(int table_id, pagenum_t pagenum, int cnt = 1);


static queue <pagenum_t> q;
void buffer_print_tree(int table_id, bool verbose = false);
void buffer_print_table(void);
void buffer_print(void);
#endif //__BUFFER_H__
