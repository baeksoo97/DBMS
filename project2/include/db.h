#ifndef __DB_H__
#define __DB_H__

#include <stdio.h>
#include <unistd.h> // access function
#include <fcntl.h> // open
#include <string.h> // strcpy, memcpy
#include "file.h"

#define MAX 1000

// Open existing data file using 'pathname' or create one if not existed
int open_table(const char *pathname);

// Insert input 'key/value' (record) to data file at the right place
int db_insert(key_t key, char * value);
record * make_record(key_t key, char * value);
page_t * make_general_page(void);
page_t * make_internal_page(void);
page_t * make_leaf_page(void);
int db_insert_new_tree(key_t key, record * pointer);
int db_insert_into_leaf(pagenum_t leaf_pagenum, page_t * leaf, key_t key, record * pointer);
int db_insert_into_leaf_after_splitting(pagenum_t leaf_pagenum, page_t * leaf, key_t key, record * pointer);
int db_insert_into_parent(pagenum_t left_pagenum, page_t * left, key_t key, pagenum_t right_pagenum, page_t * right);
int db_insert_into_new_root(pagenum_t left_pagenum, page_t * left, key_t key, pagenum_t right_pagenum, page_t * right);
int db_insert_into_node(pagenum_t n_pagenum, page_t * n,
                        int left_index, key_t key, pagenum_t right_pagenum);
int db_insert_into_node_after_splitting(pagenum_t old_pagenum, page_t * old_page, int left_index,
        key_t key, pagenum_t right_pagenum);
int db_get_left_index(page_t * parent, pagenum_t left_pagenum);
int db_cut(int length);

// Find the record containing input 'key'
int db_find(key_t key, char * ret_val);
pagenum_t db_find_leaf(key_t key);

// Find the matching record and delete it if found
int db_delete(key_t key);
int db_delete_entry(pagenum_t key_leaf_pagenum, key_t key);
pagenum_t db_remove_entry_from_page(pagenum_t n_pagenum, page_t * n_page, key_t key);
int db_adjust_root(pagenum_t root_pagenum);
int db_get_neighbor_index(pagenum_t n_pagenum, page_t * n_page, pagenum_t parent_pagenum, page_t * parent);
int db_coalesce_nodes(pagenum_t parent_pagenum, page_t * parent, pagenum_t n_pagenum, page_t * n_page,
                      pagenum_t neighbor_pagenum, page_t * neighbor, int neighbor_index,
                      key_t k_prime);
int db_redistribute_nodes(pagenum_t parent_pagenum, page_t * parent,
                          pagenum_t n_pagenum, page_t * n_page,
                          pagenum_t neighbor_pagenum, page_t * neighbor,
                          int neighbor_index, int k_prime_index, key_t k_prime);

void db_print_tree();
int IsEmpty();
int IsFull();
void db_enqueue(pagenum_t pagenum);
pagenum_t db_dequeue();
#endif //__DB_H__
