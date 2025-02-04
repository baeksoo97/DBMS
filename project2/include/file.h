#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include <stdlib.h> // uint64_t
#include <unistd.h> // lseek, write, open
#include <fcntl.h> // open
#include <string.h>
#include <string>
#include <map>
using namespace std;

#define PAGE_SIZE 4096
#define PAGE_NUM_FOR_RESERVE 10000
#define HEADER_PAGE_RESERVED_SIZE 4072
#define FREE_PAGE_RESERVED_SIZE 4088
#define PAGE_HEADER_RESERVED_SIZE 104
#define VALUE_SIZE 120
#define INTERNAL_ORDER 248
#define LEAF_ORDER 31

// Types.

typedef int64_t k_t;
typedef uint64_t pagenum_t;

struct record {
    k_t key;
    char value[VALUE_SIZE];
};

struct entry {
    k_t key;
    pagenum_t pagenum;
};

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
        struct entry entry[INTERNAL_ORDER];
        struct record record[LEAF_ORDER];
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

extern int FILE_ID;
extern map<string, int> file_list;
extern page_t * header_page;

// FUNCTION PROTOTYPES

// Try to open file named pathname and init header
int file_open(const char * pathname);

// close file named pathname
void file_close(const char * pathname);

// Create a new page
page_t * make_page();

// Free page
void free_page(page_t * page);

// Initialize header in file
void file_init_header(int isExist);

// Read header from file
page_t * header();

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t * dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t * src);

#endif //__FILE_H__
