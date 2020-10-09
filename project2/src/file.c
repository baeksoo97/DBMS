#include "file.h"

int table_id = -1;
int file_id = -1;
page_t * header_page;

void file_init_pages(){
    // init header
    printf("file id %d\n", file_id);

    file_init_header();
    printf("file init header end\n");

    for(int i = 0; i < 20; i++){
        printf("alloc %lld ", file_alloc_page());
        header_page = header();
        printf(" : header %lld %lld %lld \n", header_page->h.free_pagenum, header_page->h.root_pagenum, header_page->h.num_pages);
    }
    file_free_page(10);
    header_page = header();
    printf(" : header %lld %lld %lld \n", header_page->h.free_pagenum, header_page->h.root_pagenum, header_page->h.num_pages);

    printf("alloc %lld ", file_alloc_page());
    header_page = header();
    printf(" : header %lld %lld %lld \n", header_page->h.free_pagenum, header_page->h.root_pagenum, header_page->h.num_pages);

}

page_t * make_page(){
    page_t * page = malloc(sizeof(page_t));
    if (page == NULL){
        perror("Page creation.");
        exit(EXIT_FAILURE);
    }
    return page;
}

page_t * header(){
    file_read_page(0, header_page);
    return header_page;
}

void file_init_header(){
    header_page = make_page();

    header_page->h.free_pagenum = 0;
    header_page->h.root_pagenum = 0;
    header_page->h.num_pages = 1;
    file_write_page(0, header_page);
}

void file_init_root(const page_t * root){
    pagenum_t root_pagenum = file_alloc_page();

    header_page->h.root_pagenum = root_pagenum;

    file_write_page(root_pagenum, root);
    file_write_page(0, header_page);
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){
    header_page = header();
    // Allocate an on-disk page more
    if (header_page->h.free_pagenum == 0){
        off_t file_size = lseek(file_id, 0, SEEK_END);

        // Guarantee the size of file to be the unit of PAGE_SIZE(4096)
        if (file_size % PAGE_SIZE != 0){
            file_size = (file_size / PAGE_SIZE + 1) * PAGE_SIZE;
            if (ftruncate(file_id, file_size)){
                printf("Error : it cannot truncate to size %lld\n", file_size);
                return -1;
            }
        }

        page_t * page = make_page();
        pagenum_t start_free_pagenum = file_size / PAGE_SIZE;
        pagenum_t curr_free_pagenum = start_free_pagenum;
        for(int i = 0; i < PAGE_NUM_FOR_RESERVE; i++){
            if (i < PAGE_NUM_FOR_RESERVE - 1)
                page->f.next_free_pagenum = curr_free_pagenum + 1;
            else
                page->f.next_free_pagenum = 0; // the end of free page list

            file_write_page(curr_free_pagenum, page);
            curr_free_pagenum++;
        }
        free(page);

        pagenum_t added_pagenum = curr_free_pagenum - start_free_pagenum;

        header_page->h.free_pagenum = start_free_pagenum;
        header_page->h.num_pages += added_pagenum;
    }

    pagenum_t free_pagenum = header_page->h.free_pagenum;
    page_t * free_page = make_page();

    // Get free page from on-disk
    file_read_page(free_pagenum, free_page);

    // Update header page to on-disk
    header_page->h.free_pagenum = free_page->f.next_free_pagenum;
    file_write_page(0, header_page);

    free(free_page);

    printf("Alloc Page %lld (h.free_pagenum %lld)\n", free_pagenum, header()->h.free_pagenum);

    return free_pagenum;
}

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
    header_page = header();

    page_t * free_page = make_page();
    free_page->f.next_free_pagenum = header_page->h.free_pagenum;
    header_page->h.free_pagenum = pagenum;
//    printf("file_free_page %lld\n", header_page->h.free_pagenum);

    file_write_page(pagenum, free_page);
    file_write_page(0, header_page);

    free(free_page);
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
//    printf("file_read_page %lld : %d\n", pagenum, sizeof(*dest));
    lseek(file_id, pagenum * PAGE_SIZE, SEEK_SET);
    ssize_t read_size = read(file_id, dest, sizeof(*dest));
    if (read_size != 4096)
        printf("ERROR READ_PAGE : pagenum %lld: read size %zd\n", pagenum, read_size);
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
    off_t file_size = lseek(file_id, 0, SEEK_END);

    lseek(file_id, pagenum * PAGE_SIZE, SEEK_SET);
    ssize_t write_size = write(file_id, src, sizeof(*src));

    if (write_size != 4096)
        printf("ERROR WRITE_PAGE : pagenum %lld: read size %zd\n", pagenum, write_size);
//    printf("write pagenum %lld (size %zd) : %lld ", pagenum, write_size, file_size);
    file_size = lseek(file_id, 0, SEEK_END);
//    printf("-> %lld\n", file_size);
}

void print_file(){
    page_t page;
    printf("size %lu\n", (unsigned long)sizeof(page));
    printf("header\n");
    printf(" %lu\n", (unsigned long)&page.h.free_pagenum);
    printf(" %lu\n", (unsigned long)&page.h.root_pagenum);
    printf(" %lu\n", (unsigned long)&page.h.num_pages);
    printf(" %lu\n", (unsigned long)&page.h.reserved);
    printf(" %lu\n", (unsigned long)sizeof(page.h));
    printf("free\n");
    printf(" %lu\n", (unsigned long)&page.f.next_free_pagenum);
    printf(" %lu\n", (unsigned long)sizeof(page.f));
    printf("general\n");
    printf(" %lu\n", (unsigned long)&page.g.parent);
    printf(" %lu\n", (unsigned long)&page.g.is_leaf);
    printf(" %lu\n", (unsigned long)&page.g.num_keys);
    printf(" %lu\n", (unsigned long)&page.g.reserved);
    printf(" %lu\n", (unsigned long)&page.g.next);
    printf(" %lu\n", (unsigned long)&page.g.entry);
    printf(" %lu\n", (unsigned long)&page.g.record);
    printf(" %lu\n", (unsigned long)sizeof(page.g));
    printf(" %lu\n", (unsigned long)sizeof(page.g.entry));
    printf(" %lu\n", (unsigned long)sizeof(page.g.record));

}