# File, Page, Record Layout

## File Layout
A data file is a collection of pages, each containing a collection of records.
![bpt_file_layout](uploads/64363ff66f3e06d7863dc9ef2218713b/bpt_file_layout.png)


### How to access page in file

To access pages in disk, we should know the accurate positions of pages in disk. Because of that all of pages have the same fixed size, 4096 bytes; we can access a certain page in disk by calling system call using the physical position of page which is `page number * 4096 bytes`. For example, if we want to read some page having number 5, we can reposition the offset of file to be set 20480(5 * 4096) bytes by calling `lseek()` system call, and then read up to 4096 bytes from the offset.

![bpt_page_layout](uploads/dc90a9a5cd9971cbca163e1ec5de2bd3/bpt_page_layout.png)

## Page Layout

There are four different kinds of pages in the data file : header page, free page, internal page, leaf page. Each page has different attributes, but is managed by one structure called `page_t`.

### Page Structure

A page structure, `page_t`, is a structure for four different pages. It can be a structure for `header page`, `free page`, `internal page`, or  `leaf page`. Also,  `uint8_t reserved[PAGE_SIZE];` guarantees the size of page to be fixed in 4096 bytes by the property of **union structure**. 

```cpp
#define PAGE_SIZE 4096

typedef struct page_t {
    // in-memory page structure
    union{
        uint8_t reserved[PAGE_SIZE]; // 4096 bytes
        header_page_t h;
        free_page_t f;
        general_page_t g;
    };
} page_t;
```

### Header Page Structure

A header page is a page for meta details about the data file. It stores the number of pages and references to free page and root page. Also, `uint8_t reserved[HEADER_PAGE_RESERVED_SIZE];` guarantees the size of header page to be fixed in 4096 bytes. 

```cpp
#define HEADER_PAGE_RESERVED_SIZE 4072

typedef struct header_page_t{
    pagenum_t free_pagenum; // 8 bytes. if no free page, it is 0.
    pagenum_t root_pagenum; // 8 bytes. if empty, it is 0.
    uint64_t num_pages;     // 8 bytes
    uint8_t reserved[HEADER_PAGE_RESERVED_SIZE]; // 4072bytes
} header_page_t;
```

### Free Page Structure

A free page is an empty page not used currently, but waiting for being used in the data file. It stores a reference to next free page. Also, `uint8_t reserved[FREE_PAGE_RESERVED_SIZE];` guarantees the size of free page to be fixed in 4096 bytes. 

```cpp
#define FREE_PAGE_RESERVED_SIZE 4088

typedef struct free_page_t{
    pagenum_t next_free_pagenum; // 8bytes. if last free page, it is 0.
    uint8_t reserved[FREE_PAGE_RESERVED_SIZE]; // 4088bytes
} free_page_t;
```

### General Page Structure for Internal & Leaf

A general page is a page for internal and leaf. It has a header for meta details in the general page such as the number of keys, a reference to parent, and entries for the internal page or records for the leaf page. Also, `uint8_t reserved[PAGE_HEADER_RESERVED_SIZE];` guarantees the size of header to be fixed in 128 bytes. 

```cpp
#define PAGE_HEADER_RESERVED_SIZE 104
#define INTERNAL_ORDER 248
#define LEAF_ORDER 31

typedef struct general_page_t{
    pagenum_t parent;  // 8bytes
    uint32_t is_leaf;  // 4bytes
    uint32_t num_keys; // 4bytes
    uint8_t reserved[PAGE_HEADER_RESERVED_SIZE]; // 104bytes
    pagenum_t next;    // 8bytes
		// internal indicates the first child, leaf page indicates the right sibiling

    union{
        entry entry[INTERNAL_ORDER];
        record record[LEAF_ORDER];
    };
} general_page_t;
```

- An **internal page** is a page for indexing.
    - `parent` : a page number of parent page (if a root page, it is 0)
    - `is_leaf` : 0
    - `num_keys` : the number of entries
    - `next` : a page number of the first child page
    - `entry` : a collection of entry containing a page number of child pages and index → the total number of entries is 248(`INTERNAL_ORDER`)
- A **leaf page** is a page to store records.
    - `parent` : a page number of parent page (if a root page, it is 0)
    - `is_leaf` : 1
    - `num_keys` : the number of records
    - `next` : a page number of the next leaf page (if a rightmost leaf page, it is 0)
    - `record` : a collection of record containing the actual data(value) and key for index → the total number of records is 31(`LEAF_ORDER`)

## Record Layout

Each record in a leaf page and each entry in a internal page has some fixed type. 

### Entry Structure

An internal page has 248 entries, each containing a index key for routing to left, right child page and a page number to child page. The size of entry is fixed in 120 bytes.

```cpp
typedef int64_t k_t;
struct entry {
    k_t key;
    pagenum_t pagenum;
};
```

### Record Structure

A leaf page has 31 records, each containing a key and the actual value. The size of record is fixed in 120 bytes.

```cpp
struct record {
    k_t key;
    char value[VALUE_SIZE];
};
```