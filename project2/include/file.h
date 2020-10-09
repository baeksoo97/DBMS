#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include <stdlib.h> // uint64_t
#include <unistd.h> // lseek, write, open

#define PAGE_SIZE 4096
#define PAGE_NUM_FOR_RESERVE 10
#define HEADER_PAGE_RESERVED_SIZE 4072
#define FREE_PAGE_RESERVED_SIZE 4088
#define PAGE_HEADER_RESERVED_SIZE 104
#define VALUE_SIZE 120
#define INTERNAL_ORDER 248
#define LEAF_ORDER 31

// data types
typedef uint64_t key_t;
typedef uint64_t pagenum_t;

typedef struct record {
    key_t key;
    char value[VALUE_SIZE];
} record;

typedef struct entry {
    key_t key;
    pagenum_t pagenum;
} entry;

typedef struct header_page_t{
    pagenum_t free_pagenum;
    pagenum_t root_pagenum;
    uint64_t num_pages;
    uint8_t reserved[HEADER_PAGE_RESERVED_SIZE];
} header_page_t;

typedef struct free_page_t{
    pagenum_t next_free_pagenum;
    uint8_t reserved[FREE_PAGE_RESERVED_SIZE];
} free_page_t;

typedef struct general_page_t{
    pagenum_t parent;
    uint32_t is_leaf;
    uint32_t num_keys;
    uint8_t reserved[PAGE_HEADER_RESERVED_SIZE];
    pagenum_t next; // internal indicates the first child, leaf page indicates the right sibiling

    union{
        entry entry[INTERNAL_ORDER];
        record record[LEAF_ORDER];
    };
} general_page_t;

typedef struct page_t {
    // in-memory page structure
    union{
        uint8_t reserved[PAGE_SIZE];
        header_page_t h;
        free_page_t f;
        general_page_t g;
    };
} page_t;

// GLOBALS
extern int file_id;
extern int table_id;
extern page_t * header_page;

// Create a new page
page_t * make_page();

// Free page
void free_page(page_t * page);

page_t * header();
// Initialize header in file
void file_init_header();

void file_init_root(const page_t * root);

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t * dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t * src);

void print_file();
#endif //__FIM_H__
