#ifndef __FIM_H__
#define __FIM_H__

#include <stdlib.h>
#include <stdio.h>
typedef uint64_t pagenum_t;
typedef struct page_t {
    int a;
    // in-memory page structure
} page_t;

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src);

void print_file();
#endif //__FIM_H__
