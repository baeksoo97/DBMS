#include "buffer.h"

int buffer_init_db(int num_buf){
    if (buffer_header.buf_capacity != 0){
        printf("ERROR BUFFER_INIT_DB : already have buffer\n");
        return -1;
    }

    if (num_buf == 0){
        printf("ERROR BUFFER_INIT_DB : num_buf is 0\n");
        return -1;
    }

    buffer = (buffer_t *)malloc(num_buf * sizeof(buffer_t));
    if (buffer == NULL){
        perror("ERROR BUFFER_INIT_DB");
        return -1;
    }

    for(int i = 0; i < num_buf; i++){
        buffer[i].frame = make_page();
        buffer[i].table_id = NONE;
        buffer[i].pagenum = 0;
        buffer[i].is_dirty = false;
        buffer[i].pin_cnt = 0;
        buffer[i].prev = NONE;
        buffer[i].next = NONE;
    }

    buffer_header.buf_capacity = num_buf;
    buffer_header.frame_map.resize(TABLE_NUM);
    buffer_header.head = NONE;
    buffer_header.tail = NONE;
    buffer_header.is_open = true;

    return 0;
}

int buffer_open_table(const char * pathname){
    return file_open_table(pathname);
}

void buffer_close_db(void){
    if (!buffer_header.is_open){
        return;
    }

    for(int i = 0; i < buffer_header.buf_capacity; i++){
        free(buffer[i].frame);
    }
    free(buffer);

    for(int i = 1; i < TABLE_NUM; i++){
        buffer_header.frame_map[i].clear();
    }

    buffer_header.frame_map.clear();
    buffer_header.buf_capacity = 0;
    buffer_header.buf_size = 0;
    buffer_header.head = NONE;
    buffer_header.tail = NONE;
    buffer_header.is_open = false;
}

void buffer_flush_frame(framenum_t frame_idx){
    int table_id;
    pagenum_t pagenum;
    frame_map_t frame_map;
    frame_map_t::iterator frame_pos;

    table_id = buffer[frame_idx].table_id;
    if (table_id == -1) return;

    if (buffer[frame_idx].is_dirty){
        pagenum = buffer[frame_idx].pagenum;
        file_write_page(table_id, pagenum, buffer[frame_idx].frame);
    }

    buffer[frame_idx].table_id = NONE;
    buffer[frame_idx].pagenum = 0;
    buffer[frame_idx].is_dirty = false;
    buffer[frame_idx].pin_cnt = 0;
}

void buffer_flush_table(int table_id){
    if (!buffer_header.is_open) return;

    framenum_t frame_idx;
    frame_map_t::iterator i;
    frame_map_t frame_map = buffer_header.frame_map[table_id];

    for(i = frame_map.begin(); i != frame_map.end(); i++){
        frame_idx = i->second;
        buffer_flush_frame(frame_idx);
    }

    buffer_header.frame_map[table_id].clear();

}

int buffer_close_table(int table_id){
    framenum_t i;

    // flush all tables & close buffer
    if (table_id == 0){
        for(i = 0; i < buffer_header.buf_size; i++){
            buffer_flush_frame(i);
        }
        buffer_close_db();
    }
    // flush one table
    else{
        buffer_flush_table(table_id);
    }

    // close files
    return file_close_table(table_id);
}

int buffer_is_opened(int table_id){
    return file_is_opened(table_id);
}

// Read header from buffer
page_t * buffer_read_header(int table_id){
    page_t * header_page = make_page();
    buffer_read_page(table_id, 0, header_page);
    return header_page;
}

pagenum_t buffer_alloc_page(int table_id){
    return file_alloc_page(table_id);
}

void buffer_free_page(int table_id, pagenum_t pagenum){
    file_free_page(table_id, pagenum);
}

// insert frame_idx into the first of lru list
void buffer_lru_insert(framenum_t frame_idx){
    if (buffer_header.head == NONE && buffer_header.tail == NONE){
//        printf("// BUFFER_LRU_INSERT : empty frame_idx %d ", frame_idx);
        buffer_header.head = frame_idx;
        buffer_header.tail = frame_idx;
        buffer[frame_idx].prev = frame_idx;
        buffer[frame_idx].next = frame_idx;
    }
    else{
//        printf("// BUFFER_LRU_INSERT : insert frame_idx %d ", frame_idx);
        buffer[frame_idx].prev = buffer_header.tail;
        buffer[frame_idx].next = buffer_header.head;
        buffer[buffer_header.head].prev = frame_idx;
        buffer[buffer_header.tail].next = frame_idx;
        buffer_header.head = frame_idx;
    }
}

// update lru list
void buffer_lru_update(framenum_t frame_idx){
    // frame_idx is already mru
    if (!buffer_header.is_open)
        return;

    if (buffer_header.head == frame_idx)
        return;

//    printf("// BUFFER_LRU_UPDATE %d ", frame_idx);
    if (buffer_header.tail == frame_idx){
        buffer_header.tail = buffer[frame_idx].prev;
    }

    // get frame_idx out from list
    buffer[buffer[frame_idx].prev].next = buffer[frame_idx].next;
    buffer[buffer[frame_idx].next].prev = buffer[frame_idx].prev;
    buffer[frame_idx].next = buffer_header.head;
    buffer[frame_idx].prev = buffer_header.tail;
    buffer[buffer_header.head].prev = frame_idx;
    buffer[buffer_header.tail].next = frame_idx;
    buffer_header.head = frame_idx;
}

framenum_t buffer_lru_frame(void){
    framenum_t frame_idx;
    int table_id;
    pagenum_t pagenum;
    frame_map_t::iterator frame_pos;

    frame_idx = buffer_header.tail;

    while(true){
        if(buffer[frame_idx].pin_cnt == 0) break;
        if(buffer[frame_idx].prev == buffer_header.head){
            printf("BUFFER_LRU_FRAME : all of frames in buffer is being used\n");
            break;
        }
        frame_idx = buffer[frame_idx].prev;
    }

    // remove <pagenum, frame_idx> from frame_map
    table_id = buffer[frame_idx].table_id;
    if (table_id != -1){
        pagenum = buffer[frame_idx].pagenum;
        frame_pos = buffer_header.frame_map[table_id].find(pagenum);
        buffer_header.frame_map[table_id].erase(frame_pos);

        buffer_flush_frame(frame_idx);
    }

//    printf("// BUFFER_FLUSH_FRAME table_id %d pagenum %lu frame_idx %d", table_id, pagenum, frame_idx);

    return frame_idx;
}

// get frame_idx from buffer which can will be used
framenum_t buffer_alloc_frame(void){
    framenum_t frame_idx;

    // alloc empty frame
    if (buffer_header.buf_size < buffer_header.buf_capacity){
        frame_idx = buffer_header.buf_size++;
        buffer_lru_insert(frame_idx);
//        printf("// BUFFER_ALLOC empty frame %d ", frame_idx);
    }
    // alloc the least recently used frame
    else{
        frame_idx = buffer_lru_frame();
        buffer_lru_update(frame_idx);
//        printf("// BUFFER_ALLOC lru frame %d ", frame_idx);
    }

    return frame_idx;
}

int buffer_init_frame(int table_id, pagenum_t pagenum, page_t * dest){
    // get the index of frame which page would be inserted
    framenum_t frame_idx = buffer_alloc_frame();

    memcpy(buffer[frame_idx].frame, dest, sizeof(page_t));

    buffer[frame_idx].table_id = table_id;
    buffer[frame_idx].pagenum = pagenum;
    buffer[frame_idx].is_dirty = 0;
    buffer[frame_idx].pin_cnt = 0;

    buffer_header.frame_map[table_id][pagenum] = frame_idx;

    return frame_idx;
}

framenum_t buffer_find_frame(int table_id, pagenum_t pagenum){
    frame_map_t frame_map;
    framenum_t frame_idx;

    frame_map = buffer_header.frame_map[table_id];

    if (frame_map.count(pagenum) == 0) return -1;
    if (frame_map[pagenum] == NONE) return -1;

    frame_idx = frame_map[pagenum];

    return frame_idx;
}

void buffer_read_page(int table_id, pagenum_t pagenum, page_t * dest){
    if (!buffer_header.is_open)
        return file_read_page(table_id, pagenum, dest);

    framenum_t frame_idx = buffer_find_frame(table_id, pagenum);

//    printf("BUFFER_READ_PAGE  : table_id %d, pagenum %lu, frame idx %d ", table_id, pagenum, frame_idx);

    // if buffer doesn't have page
    if (frame_idx == -1){
        file_read_page(table_id, pagenum, dest);
        frame_idx = buffer_init_frame(table_id, pagenum, dest);
        miss_cnt++;
//        printf("// read from disk\n");
    }
    // if buffer already has page
    else{
        memcpy(dest, buffer[frame_idx].frame, sizeof(page_t));
        buffer_lru_update(frame_idx);
        hit_cnt++;
//        printf("// read from buffer\n");
    }

    buffer[frame_idx].pin_cnt++;
}

void buffer_write_page(int table_id, pagenum_t pagenum, page_t * src){
    if (!buffer_header.is_open)
        return file_write_page(table_id, pagenum, src);

    framenum_t frame_idx = buffer_find_frame(table_id, pagenum);

//    printf("BUFFER_WRITE_PAGE : table_id %d, pagenum %lu, frame idx %d ", table_id, pagenum, frame_idx);

    // if buffer doesn't have page
    if (frame_idx == -1){
        frame_idx = buffer_init_frame(table_id, pagenum, src);
        miss_cnt++;
//        printf("// init frame\n");
    }
    // if buffer already has page
    else{
        memcpy(buffer[frame_idx].frame, src, sizeof(page_t));
        buffer_lru_update(frame_idx);
        hit_cnt++;
//        printf("// write to buffer\n");
    }
    buffer[frame_idx].is_dirty = true;
    buffer[frame_idx].pin_cnt++;
}

void buffer_unpin_frame(int table_id, pagenum_t pagenum, int cnt){
    if (!buffer_header.is_open) return;

    framenum_t frame_idx = buffer_find_frame(table_id, pagenum);

    if (frame_idx == -1){
        printf("BUFFER_UNPIN_FRAME frame_idx = -1 : table_id %d, pagenum %lu\n", table_id, pagenum);
        return;
    }

    buffer[frame_idx].pin_cnt -= cnt;
//    printf("BUFFER_UNPIN_FRAME table_id %d pagenum %lu cnt %d\n", table_id, pagenum, buffer[frame_idx].pin_cnt);
}

// OUTPUT

void buffer_print(void){
    if (hit_cnt == 0 && miss_cnt == 0) return;
    double ratio = (double)hit_cnt / (miss_cnt + hit_cnt) * 100;
    printf("miss_cnt %d hit_cnt %d ratio %lf%%\n", miss_cnt, hit_cnt, ratio);

    if (!buffer_header.is_open) {
        printf("Buffer not opened\n");
        return;
    }

    int i;
    frame_map_t::iterator j;
    frame_map_t frame_map;
    printf("----------------------------------------BUFFER---------------------------------------\n");
    printf("buffer head %d / tail %d\n", buffer_header.head, buffer_header.tail);
    for(i = 0; i < buffer_header.buf_capacity; i++){
        printf("frame_idx %d : table_id %d / pagenum %lu / is_dirty %d / ", i, buffer[i].table_id, buffer[i].pagenum, buffer[i].is_dirty);
        printf("pin_cnt %d / prev %d / next %d\n", buffer[i].pin_cnt, buffer[i].prev, buffer[i].next);
    }

    printf("----------------------------------------MAPMAP---------------------------------------\n");
    for(i = 1; i < TABLE_NUM; i++){
        frame_map = buffer_header.frame_map[i];
        printf("table_id %3d : ", i);
        for(j = frame_map.begin(); j != frame_map.end(); j++){
            printf("(%lu, %d) ", j->first, j->second);
        }
        printf("\n");
    }

//    printf("---- LRU list ");
//    while(true){
//        printf("-> %d ", frame_idx);
//        if (frame_idx == buffer_header.tail)
//            break;
//        frame_idx = buffer[frame_idx].next;
//    }
//    printf("\n");
}

// Print tree from disk by calling 'file_print_tree()'
void buffer_print_tree(int table_id, bool verbose){
    file_print_tree(table_id, verbose);
}

// Print file_name & table_id & fd map by calling 'file_print_table()'
void buffer_print_table(void){
    file_print_table();
}