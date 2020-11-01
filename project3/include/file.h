#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include <stdlib.h> // uint64_t
#include <unistd.h> // lseek, write, open
#include <fcntl.h> // open
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <queue>
using namespace std;

#define PAGE_SIZE 4096
#define PAGE_NUM_FOR_RESERVE 1000
#define HEADER_PAGE_RESERVED_SIZE 4072
#define FREE_PAGE_RESERVED_SIZE 4088
#define PAGE_HEADER_RESERVED_SIZE 104
#define VALUE_SIZE 120
#define INTERNAL_ORDER 248
#define LEAF_ORDER 31
#define TABLE_NUM 11

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

static map<string, int> file_table_map;
static vector<pair<bool, int> > table_fd_map(TABLE_NUM, make_pair(false, 0)); // table_id : 1 ~ 10

// FUNCTION PROTOTYPES

// Try to open file named pathname and init header
int file_set_table(const char * pathname, int fd);
int file_open_table(const char * pathname);

// Write the pages relating to this table to disk and close the table
int close_file(int table_id);
int file_close_table(int table_id);

int file_is_opened(int table_id);
int get_file_id(int table_id);

// Create a new page
page_t * make_page();

// Free page
void free_page(page_t * page);

// Initialize header in file
void file_init_header(int table_id);

// Read header from file
page_t * file_read_header(int table_id);

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int table_id);

// Free an on-disk page to the free page list
void file_free_page(int table_id, pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int table_id, pagenum_t pagenum, page_t * dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(int table_id, pagenum_t pagenum, const page_t * src);

// Print tree from disk
static queue <pagenum_t> q;
void file_print_tree(int table_id, bool verbose = false);

// Print file_name & table_id & fd map
void file_print_table(void);

#endif //__FILE_H__
