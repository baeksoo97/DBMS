#ifndef __INDEX_H__
#define __INDEX_H__

#include <stdio.h>
#include <string.h> // strcpy, memcpy
#include "buffer.h"
using namespace std;

// FUNCTION PROTOTYPES.

// Utility.

int index_open_table(const char * pathname);

int index_init_db(int buf_num);

int index_close_table(int table_id = 0);

int index_is_opened(int table_id);

// Find.

pagenum_t find_leaf(int table_id, pagenum_t root_pagenum, k_t key);
int find(int table_id, pagenum_t root_pagenum, k_t key, char * ret_val);
int _find(int table_id, k_t key, char * ret_val);
int trx_find(int table_id, k_t key, char * ret_val, int trx_id);

// Update.

int trx_update(int table_id, k_t key, char * value, int trx_id);

// Insertion.

struct record * make_record(k_t key, char * value);
page_t * index_make_page(void);
page_t * make_general_page(void);
page_t * make_internal_page(void);
page_t * make_leaf_page(void);
int cut(int length);
int get_left_index(page_t * parent, pagenum_t left_pagenum);
int insert_into_page_after_splitting(int table_id, pagenum_t old_pagenum, page_t * old_page,
                                     int left_index, k_t key, pagenum_t right_pagenum);
int insert_into_page(int table_id, pagenum_t n_pagenum, page_t * n,
                     int left_index, k_t key, pagenum_t right_pagenum);
int insert_into_parent(int table_id, pagenum_t left_pagenum, page_t * left,
                       k_t key, pagenum_t right_pagenum, page_t * right);
int insert_into_leaf_after_splitting(int table_id, pagenum_t leaf_pagenum, page_t * leaf,
                                     k_t key, struct record * pointer);
int insert_into_leaf(int table_id, pagenum_t leaf_pagenum, page_t * leaf, k_t key, struct record * pointer);
int insert_into_new_root(int table_id, pagenum_t left_pagenum, page_t * left,
                         k_t key, pagenum_t right_pagenum, page_t * right);
int insert_new_tree(int table_id, k_t key, struct record * pointer);
int insert(int table_id, k_t key, char * value);

// Deletion.

int redistribute_nodes(int table_id, pagenum_t parent_pagenum, page_t * parent,
                       pagenum_t n_pagenum, page_t * n_page,
                       pagenum_t neighbor_pagenum, page_t * neighbor,
                       int neighbor_index, int k_prime_index, k_t k_prime);
int coalesce_nodes(int table_id, pagenum_t parent_pagenum, page_t * parent,
                   pagenum_t n_pagenum, page_t * n_page,
                   pagenum_t neighbor_pagenum, page_t * neighbor,
                   int neighbor_index, k_t k_prime);
int get_neighbor_index(pagenum_t n_pagenum, page_t * n_page,
                       pagenum_t parent_pagenum, page_t * parent);
int adjust_root(int table_id, pagenum_t root_pagenum);
pagenum_t remove_entry_from_page(int table_id, pagenum_t n_pagenum, page_t * n_page, k_t key);
int delete_entry(int table_id, pagenum_t key_leaf_pagenum, k_t key);
int delete_key(int table_id, k_t key);

// Output.

void index_print_tree(int table_id, bool verbose = false);
void index_print_table(void);
void index_print_buffer(void);

#endif //__INDEX_H__
