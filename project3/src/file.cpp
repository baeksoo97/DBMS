#include "file.h"
#include "buffer.h"

// Function Definition

// Utility

// Helper function for file_open_table() : set table_id of data file
int file_set_table(const char * pathname, int fd){
    int table_id;
    string s_pathname(pathname);
    if (!file_table_map.count(s_pathname)){
        table_id = (int)file_table_map.size() + 1;

        if (table_id >= TABLE_NUM){
            printf("ERROR FILE_OPEN_TABLE : all of table_ids are already used\n");
            return -1;
        }

        file_table_map[s_pathname] = table_id;
        table_fd_map[table_id] = make_pair(true, fd);
    }
    else{
        table_id = file_table_map[s_pathname];
        table_fd_map[table_id] = make_pair(true, fd);
    }

    return table_id;
}

// Try to open file named pathname and init header
int file_open_table(const char * pathname){
    int fd, table_id;

    if (!access(pathname, F_OK)){
        // file exists
        fd = open(pathname, O_RDWR, 0644); // open, read, write
        if (fd == -1){
            perror("ERROR FILE_OPEN : cannot open file\n");
            return -1;
        }
//        printf("open already existing file : %d\n", fd);

        table_id = file_set_table(pathname, fd);
    }
    else{
        // file doesn't exist
        if ((int)file_table_map.size() >= TABLE_NUM - 1){
            printf("ERROR FILE_OPEN : num of tables can be made up to 10\n");
            return -1;
        }

        fd = open(pathname, O_CREAT | O_RDWR, 0644); // create new, read, write
        if (fd == -1){
            perror("ERROR FILE_OPEN : cannot create file\n");
            return -1;
        }
//        printf("create new file : %d\n", fd);

        table_id = file_set_table(pathname, fd);
        file_init_header(table_id);
    }

    return table_id;
}

int close_file(int table_id) {
    int fd;
    fd = table_fd_map[table_id].second;
    if (close(fd)){
        perror("ERROR FILE_CLOSE_TABLE");
        return -1;
    }

    table_fd_map[table_id].first = false;
    table_fd_map[table_id].second = 0;
    return 0;
}

// Write the pages relating to this table to disk and close the table
int file_close_table(int table_id){
    int i;
    // shutdown_db() : Flush all data from buffer and destroy allocated buffer
    if (table_id == 0){
        for (i = 1; i < TABLE_NUM; i++){
            if (table_fd_map[i].first) {
                if (close_file(i)) return -1;
            }
        }
        file_table_map.clear();
    }
    // write all pages of this table from buffer to disk
    else{
        if (close_file(table_id)) return -1;
    }

    return 0;
}

int file_is_opened(int table_id){
    if (table_fd_map[table_id].first) return 1;
    else return 0;
}

int get_file_id(int table_id){
    return table_fd_map[table_id].second;
}

// Create a new page
page_t * make_page(){
    page_t * page = (page_t *)malloc(sizeof(page_t));
    if (page == NULL){
        perror("ERROR MAKE_PAGE : Page creation.");
        exit(EXIT_FAILURE);
    }
    return page;
}

// Free page
void free_page(page_t * page){
    free(page);
}

// Initialize header in file
void file_init_header(int table_id){
    page_t * header_page = make_page();
    header_page->h.free_pagenum = 0;
    header_page->h.root_pagenum = 0;
    header_page->h.num_pages = 1;
    buffer_write_page(table_id, 0, header_page);
    buffer_unpin_frame(table_id, 0);
    free_page(header_page);
}

// Read header from file
page_t * file_read_header(int table_id){
    page_t * header_page = make_page();
    buffer_read_page(table_id, 0, header_page);
    return header_page;
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int table_id){
//    printf("file_alloc_page\n");
    page_t * header_page, * page;
    pagenum_t start_free_pagenum, curr_free_pagenum, added_free_pagenum, pagenum;

    header_page = file_read_header(table_id);
    page = make_page();

    // Allocate an on-disk page more
    if (header_page->h.free_pagenum == 0){

        start_free_pagenum = header_page->h.num_pages;
        curr_free_pagenum = start_free_pagenum;
        for(int i = 0; i < PAGE_NUM_FOR_RESERVE; i++){
            if (i < PAGE_NUM_FOR_RESERVE - 1)
                page->f.next_free_pagenum = curr_free_pagenum + 1;
            else
                page->f.next_free_pagenum = 0; // the end of free page list

            file_write_page(table_id, curr_free_pagenum, page);
            curr_free_pagenum++;
        }

        added_free_pagenum = curr_free_pagenum - start_free_pagenum;

        header_page->h.free_pagenum = start_free_pagenum;
        header_page->h.num_pages += added_free_pagenum;
    }

    pagenum = header_page->h.free_pagenum;

    // Get free page from on-disk
    buffer_read_page(table_id, pagenum, page);

    // Update header page to on-disk
    header_page->h.free_pagenum = page->f.next_free_pagenum;
    buffer_write_page(table_id, 0, header_page);

    buffer_unpin_frame(table_id, 0, 2);
    buffer_unpin_frame(table_id, pagenum);

    // printf("Alloc Page %lld (h.free_pagenum %lld)\n", pagenum, header()->h.free_pagenum);

    free_page(header_page);
    free_page(page);

    return pagenum;
}

// Free an on-disk page to the free page list
void file_free_page(int table_id, pagenum_t pagenum){
//    printf("file_free_page\n");
    page_t * header_page, * page;

    header_page = file_read_header(table_id);
    page = make_page();

    page->f.next_free_pagenum = header_page->h.free_pagenum;
    header_page->h.free_pagenum = pagenum;

    // update header
    buffer_write_page(table_id, 0, header_page);
    buffer_unpin_frame(table_id, 0, 2);
    free_page(header_page);

    // update page
    buffer_write_page(table_id, pagenum, page);
    buffer_unpin_frame(table_id, pagenum);
    free_page(page);
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int table_id, pagenum_t pagenum, page_t* dest){
    int fd = get_file_id(table_id);

    lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
    ssize_t read_size = read(fd, dest, PAGE_SIZE);

    if (read_size == -1)
        perror("ERROR READ_PAGE");
    else if (read_size != PAGE_SIZE)
        printf("ERROR READ_PAGE : table_id %d, pagenum %llu, read size %zd\n", table_id, pagenum, read_size);
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int table_id, pagenum_t pagenum, const page_t* src){
    int fd = get_file_id(table_id);

    lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
    ssize_t write_size = write(fd, src, PAGE_SIZE);
    fsync(fd);

    if (write_size == -1)
        perror("ERROR WRITE_PAGE");
    else if (write_size != PAGE_SIZE)
        printf("ERROR WRITE_PAGE : table_id %d, pagenum %llu, write size %zd\n", table_id, pagenum, write_size);
}

// Print tree from disk
void file_print_tree(int table_id, bool verbose){
    int i;
    page_t * header_page, * page;
    pagenum_t pagenum;

    header_page = make_page();
    page = make_page();
    file_read_page(table_id, 0, header_page);

    if (header_page->h.root_pagenum == 0){
        printf("Tree is empty\n");
        printf("----------\n");
        free_page(page);
        return;
    }

    q.push(header_page->h.root_pagenum);
    while(!q.empty()){
        int temp_size = q.size();

        while(temp_size){
            pagenum = q.front();
            q.pop();

            printf("pagenum %llu ", pagenum);
            file_read_page(table_id, pagenum, page);
            if (page->g.is_leaf){
                printf("leaf : ");
                for(i = 0; i < page->g.num_keys; i++){
                    printf("(%lld, %s) ", page->g.record[i].key, page->g.record[i].value);
                }
                printf(" | ");
            }
            else{
                printf("internal : ");
                if (page->g.num_keys > 0){
                    printf("[%llu] ", page->g.next);
                    q.push(page->g.next);
                }
                for(i = 0; i < page->g.num_keys; i++){
                    printf("%llu [%llu] ", page->g.entry[i].key, page->g.entry[i].pagenum);
                    q.push(page->g.entry[i].pagenum);
                }
                printf(" | ");
            }
            temp_size--;
        }
        printf("\n");
    }

    if (verbose){
        q.push(header_page->h.root_pagenum);
        while(!q.empty()){
            pagenum = q.front();
            q.pop();

            printf("pagenum %llu ", pagenum);
            file_read_page(table_id, pagenum, page);

            if (page->g.is_leaf){
                printf("leaf pagenum : %llu, parent : %llu, is_leaf : %d, num keys : %d, right sibling : %llu",
                       pagenum, page->g.parent, page->g.is_leaf, page->g.num_keys, page->g.next);
                printf(" | ");
            }
            else{
                printf("internal pagenum : %llu, parent : %llu, is_leaf : %d, num keys : %d, one more : %llu",
                       pagenum, page->g.parent, page->g.is_leaf, page->g.num_keys, page->g.next);

                q.push(page->g.next);
                for(i = 0; i < page->g.num_keys; i++){
                    q.push(page->g.entry[i].pagenum);
                }

                printf(" | ");
            }
        }
    }
    printf("\n");

    free_page(header_page);
    free_page(page);
}

// Print file_name & table_id & fd map
void file_print_table(void){
    int i, fd;
    bool visit;
    map<string, int>::iterator it;
    printf("%10s %10s - %5s %5s\n", "table_id", "file_name", "fd", "is_open");
    for(i = 1; i <= 10; i++){
        visit = table_fd_map[i].first;
        fd = table_fd_map[i].second;
        string file_name;
        for(it = file_table_map.begin(); it != file_table_map.end(); it++){
            if (it->second == i){
                file_name = it->first;
            }
        }
        printf("%10d %10s - %5d %5d\n", i, file_name.c_str(), fd, visit);
    }
}
