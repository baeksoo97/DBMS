#include "buffer.h"

int buffer_init_db(int num_buf){
    if (buffer_header.num_buf != 0){
        printf("ERROR BUFFER_INIT_DB : already have buffer\n");
        return -1;
    }

    buffer = (buffer_t *)malloc(num_buf * sizeof(buffer_t));
    if (buffer == NULL){
        perror("ERROR BUFFER_INIT_DB");
        return -1;
    }

    for(int i = 0; i < num_buf; i++){
        buffer[i].frame = make_page();
        buffer[i].table_id = -1;
        buffer[i].pagenum = 0;
        buffer[i].is_dirty = false;
        buffer[i].pin_count = 0;
        buffer[i].prev = NULL;
        buffer[i].next = NULL;
    }

    buffer_header.num_buf = num_buf;
    buffer_header.frame_map.resize(TABLE_NUM);

    return 0;
}

void buffer_close_db(void){
    if (buffer_header.num_buf == 0){
        return;
    }

    printf("num buf %d\n", buffer_header.num_buf);
    for(int i = 0; i < buffer_header.num_buf; i++){
        free(buffer[i].frame);
    }
    free(buffer);

    buffer_header.num_buf = 0;

    for(int i = 1; i < TABLE_NUM; i++){
        buffer_header.frame_map[i].clear();
    }
    buffer_header.frame_map.clear();
}

int buffer_open_table(const char * pathname){
    return file_open_table(pathname);
}

int buffer_close_table(int table_id){
    // flush first

    // close buffer
    if (table_id == 0){
        buffer_close_db();
    }

    // close files
    return file_close_table(table_id);
}

int buffer_is_opened(int table_id){
    return file_is_opened(table_id);
}

pagenum_t buffer_alloc_page(int table_id){
    return file_alloc_page(table_id);
}

void buffer_free_page(int table_id, pagenum_t pagenum){
    file_free_page(table_id, pagenum);
}

framenum_t buffer_find_frame(int table_id, pagenum_t pagenum){
    frame_map_t frame_map = buffer_header.frame_map[table_id];

    if (frame_map.count(pagenum) == 0) return -1;
    if (frame_map[pagenum] == -1) return -1;

    framenum_t frame_idx = frame_map[pagenum];

    return frame_idx;
}

void buffer_read_page(int table_id, pagenum_t pagenum, page_t * dest){
    framenum_t frame_idx = buffer_find_frame(table_id, pagenum);
    printf("buffer read frame idx %d\n", frame_idx);

    file_read_page(table_id, pagenum, dest);
}

void buffer_write_page(int table_id, pagenum_t pagenum, const page_t * src){
    file_write_page(table_id, pagenum, src);
}