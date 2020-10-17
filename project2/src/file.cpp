#include "file.h"

int FILE_ID = -1;
map <string, int> file_list;
page_t * header_page = NULL;

// Function Definition

// Utility

// Try to open file named pathname and init header
int file_open(const char * pathname){
    int fd;

    if (!access(pathname, F_OK)){
        // file exists
        fd = open(pathname, O_RDWR, 0644); // open, read, write
        if (fd == -1){
            perror("ERROR FILE_OPEN : cannot open file\n");
            return -1;
        }
        printf("open already existing file : %d\n", fd);
        FILE_ID = fd;
        file_init_header(1);
    }
    else{
        // file doesn't exist
        fd = open(pathname, O_CREAT | O_RDWR, 0644); // create new, read, write
        if (fd == -1){
            perror("ERROR FILE_OPEN : cannot create file\n");
            return -1;
        }
        printf("create new file : %d\n", fd);
        FILE_ID = fd;
        file_init_header(0);
    }

    return fd;
}

// Close data file named pathname
void file_close(const char * pathname) {
    if (!strcmp(pathname, "")){
        // close all opened files
        if (close(FILE_ID))
            perror("ERROR FILE_CLOSE");
        else
            FILE_ID = -1;
    }
    else{
        // close file named pathname
        if (close(FILE_ID))
            perror("ERROR FILE_CLOSE");
        else{
            printf("close file %d\n", FILE_ID);
            FILE_ID = -1;
        }
    }
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
void file_init_header(int isExist){
    header_page = make_page();

    if (!isExist){
        header_page->h.free_pagenum = 0;
        header_page->h.root_pagenum = 0;
        header_page->h.num_pages = 1;
        file_write_page(0, header_page);
    }
}

// Read header from file
page_t * header(){
    file_read_page(0, header_page);
    return header_page;
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){
    pagenum_t pagenum;
    page_t * page = make_page();

    header_page = header();

    // Allocate an on-disk page more
    if (header_page->h.free_pagenum == 0){
        off_t file_size;
        pagenum_t start_free_pagenum, curr_free_pagenum, added_free_pagenum;

        file_size = lseek(FILE_ID, 0, SEEK_END);
        if (file_size == -1){
            perror("ERROR FILE_ALLOC_PAGE");
        }

        // Guarantee the size of file to be the unit of PAGE_SIZE(4096)
        if (file_size % PAGE_SIZE != 0){
            file_size = (file_size / PAGE_SIZE + 1) * PAGE_SIZE;
            if (ftruncate(FILE_ID, file_size)){
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

            file_write_page(curr_free_pagenum, page);
            curr_free_pagenum++;
        }

        added_free_pagenum = curr_free_pagenum - start_free_pagenum;

        header_page->h.free_pagenum = start_free_pagenum;
        header_page->h.num_pages += added_free_pagenum;
    }

    pagenum = header_page->h.free_pagenum;

    // Get free page from on-disk
    file_read_page(pagenum, page);

    // Update header page to on-disk
    header_page->h.free_pagenum = page->f.next_free_pagenum;
    file_write_page(0, header_page);

    // printf("Alloc Page %lld (h.free_pagenum %lld)\n", pagenum, header()->h.free_pagenum);

    free_page(page);

    return pagenum;
}

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
    page_t * page = make_page();
    header_page = header();

    page->f.next_free_pagenum = header_page->h.free_pagenum;
    header_page->h.free_pagenum = pagenum;

    file_write_page(pagenum, page);
    file_write_page(0, header_page);

    free_page(page);
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
    lseek(FILE_ID, pagenum * PAGE_SIZE, SEEK_SET);
    ssize_t read_size = read(FILE_ID, dest, PAGE_SIZE);

    if (read_size == -1)
        perror("ERROR READ_PAGE");
    else if (read_size != PAGE_SIZE)
        printf("ERROR READ_PAGE : pagenum %lu, read size %zd\n", pagenum, read_size);
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
    lseek(FILE_ID, pagenum * PAGE_SIZE, SEEK_SET);
    ssize_t write_size = write(FILE_ID, src, PAGE_SIZE);
    fsync(FILE_ID);

    if (write_size == -1)
        perror("ERROR WRITE_PAGE");
    else if (write_size != PAGE_SIZE)
        printf("ERROR WRITE_PAGE : pagenum %lu, write size %zd\n", pagenum, write_size);
}
