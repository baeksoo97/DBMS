#include "file.h"

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){
}

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
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