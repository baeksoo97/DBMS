#ifndef __INDEX_H__
#define __INDEX_H__

#include <stdio.h>
#include <string.h> // strcpy, memcpy
#include "file.h"

#define QUEUE_SIZE 100000000
// FUNCTION PROTOTYPES.

// Utility.

int index_open_table(const char * pathname);

// Find.

pagenum_t find_leaf(key_t key);
int find(key_t key, char * ret_val);

// Insertion.

record * make_record(key_t key, char * value);
page_t * make_general_page(void);
page_t * make_internal_page(void);
page_t * make_leaf_page(void);
int cut(int length);
int get_left_index(page_t * parent, pagenum_t left_pagenum);
int insert_into_page_after_splitting(pagenum_t old_pagenum, page_t * old_page,
                                     int left_index, key_t key, pagenum_t right_pagenum);
int insert_into_page(pagenum_t n_pagenum, page_t * n,
                     int left_index, key_t key, pagenum_t right_pagenum);
int insert_into_parent(pagenum_t left_pagenum, page_t * left,
                       key_t key, pagenum_t right_pagenum, page_t * right);
int insert_into_leaf_after_splitting(pagenum_t leaf_pagenum, page_t * leaf,
                                     key_t key, record * pointer);
int insert_into_leaf(pagenum_t leaf_pagenum, page_t * leaf, key_t key, record * pointer);
int insert_into_new_root(pagenum_t left_pagenum, page_t * left,
                         key_t key, pagenum_t right_pagenum, page_t * right);
int insert_new_tree(key_t key, record * pointer);
int insert(key_t key, char * value);

// Deletion.

int redistribute_nodes(pagenum_t parent_pagenum, page_t * parent,
                       pagenum_t n_pagenum, page_t * n_page,
                       pagenum_t neighbor_pagenum, page_t * neighbor,
                       int neighbor_index, int k_prime_index, key_t k_prime);
int coalesce_nodes(pagenum_t parent_pagenum, page_t * parent,
                   pagenum_t n_pagenum, page_t * n_page,
                   pagenum_t neighbor_pagenum, page_t * neighbor,
                   int neighbor_index, key_t k_prime);
int get_neighbor_index(pagenum_t n_pagenum, page_t * n_page,
                       pagenum_t parent_pagenum, page_t * parent);
int adjust_root(pagenum_t root_pagenum);
pagenum_t remove_entry_from_page(pagenum_t n_pagenum, page_t * n_page, key_t key);
int delete_entry(pagenum_t key_leaf_pagenum, key_t key);
int delete(key_t key);

// Output.

int is_empty(void);
int is_full(void);
pagenum_t dequeue(void);
void enqueue(pagenum_t pagenum);
void print_tree(void);

#endif //__INDEX_H__
