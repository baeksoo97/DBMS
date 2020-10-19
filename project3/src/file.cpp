#include "file.h"

map <string, int> file_table_map;
vector<pair<bool, int> > table_fd_map(11, make_pair(false, 0));
page_t * header_page = make_page();

// Function Definition

// Utility
bool is_table_opened(int table_id){
    if (table_id <= 0 || table_id > 10) return false;

    return table_fd_map[table_id].first;
}

int get_file_id(int table_id){
    return table_fd_map[table_id].second;
}

int get_table_id(void){
    int table_id;
    for (table_id = 1; table_id <= 10; table_id++){
        if (!table_fd_map[table_id].first)
            return table_id;
    }
    return -1;
}

// Helper function for file_open_table() : set table_id of data file
int file_set_table(const char * pathname, int fd){
    int table_id;
    string s_pathname(pathname);
    if (!file_table_map.count(s_pathname)){
        table_id = get_table_id();
        if (table_id == -1){
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
    printf("pathname %s, table_id %d, fd %d\n", pathname, table_id, fd);
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
        printf("open already existing file : %d\n", fd);

        table_id = file_set_table(pathname, fd);
    }
    else{
        // file doesn't exist
        if (get_table_id() == -1){
            printf("ERROR FILE_OPEN : num of tables can be made up to 10\n");
            return -1;
        }

        fd = open(pathname, O_CREAT | O_RDWR, 0644); // create new, read, write
        if (fd == -1){
            perror("ERROR FILE_OPEN : cannot create file\n");
            return -1;
        }
        printf("create new file : %d\n", fd);

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
    printf("close file %d\n", table_id);
    table_fd_map[table_id].first = false;
    return 0;
}

// Write the pages relating to this table to disk and close the table
int file_close_table(int table_id) {
    int i;
    if (table_id == 0){
        // shutdown_db() : Flush all data from buffer and destroy allocated buffer
        for (i = 1; i <= 10; i++){
            if (table_fd_map[i].first){
                if (close_file(i)) return -1;
            }
        }
    }
    else{
        // write all pages of this table from buffer to disk
        if (close_file(table_id)) return -1;
    }

    return 0;
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
    header_page->h.free_pagenum = 0;
    header_page->h.root_pagenum = 0;
    header_page->h.num_pages = 1;
    file_write_page(table_id, 0, header_page);
}

// Read header from file
page_t * header(int table_id){
    file_read_page(table_id, 0, header_page);
    return header_page;
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int table_id){
    int fd = get_file_id(table_id);
    pagenum_t pagenum;
    page_t * page = make_page();

    header_page = header(table_id);

    // Allocate an on-disk page more
    if (header_page->h.free_pagenum == 0){
        off_t file_size;
        pagenum_t start_free_pagenum, curr_free_pagenum, added_free_pagenum;

        file_size = lseek(fd, 0, SEEK_END);
        if (file_size == -1){
            perror("ERROR FILE_ALLOC_PAGE");
        }

        // Guarantee the size of file to be the unit of PAGE_SIZE(4096)
        if (file_size % PAGE_SIZE != 0){
            file_size = (file_size / PAGE_SIZE + 1) * PAGE_SIZE;
            if (ftruncate(fd, file_size)){
                perror("ERROR FILE_ALLOC_PAGE");
            }
        }

        start_free_pagenum = file_size / PAGE_SIZE;
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
    file_read_page(table_id, pagenum, page);

    // Update header page to on-disk
    header_page->h.free_pagenum = page->f.next_free_pagenum;
    file_write_page(table_id, 0, header_page);

    // printf("Alloc Page %lld (h.free_pagenum %lld)\n", pagenum, header()->h.free_pagenum);

    free_page(page);

    return pagenum;
}

// Free an on-disk page to the free page list
void file_free_page(int table_id, pagenum_t pagenum){
    page_t * page = make_page();
    header_page = header(table_id);

    page->f.next_free_pagenum = header_page->h.free_pagenum;
    header_page->h.free_pagenum = pagenum;

    file_write_page(table_id, pagenum, page);
    file_write_page(table_id, 0, header_page);

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
