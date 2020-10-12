#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "db.h"
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// TYPES.

// FUNCTION PROTOTYPES.

// Output and utility.

void usage_2( void );
//void enqueue( node * new_node );
//node * dequeue( void );
//int height( node * root );
//int path_to_root( node * root, node * child );
//void print_leaves( node * root );
//void print_tree( node * root );
//void find_and_print( node * root, uint64_t key, bool verbose );
//void find_and_print_range( node * root, uint64_t range1, uint64_t range2, bool verbose );
//uint64_t find_range( node * root, uint64_t key_start, uint64_t key_end, bool verbose,
//        uint64_t returned_keys[], void * returned_pointers[]);
//node * find_leaf( node * root, uint64_t key, bool verbose );
void find(key_t key);
//int cut( int length );

// Insertion.

//pointer * make_pointer(char * value);
//node * make_node( void );
//node * make_leaf( void );
//int get_left_index(node * parent, node * left);
//node * insert_into_leaf( node * leaf, uint64_t key, pointer * pointer );
//node * insert_into_leaf_after_splitting(node * root, node * leaf, uint64_t key,
//                                        pointer * pointer);
//node * insert_into_node(node * root, node * parent,
//        int left_index, uint64_t key, node * right);
//node * insert_into_node_after_splitting(node * root, node * parent,
//                                        int left_index,
//        uint64_t key, node * right);
//node * insert_into_parent(node * root, node * left, uint64_t key, node * right);
//node * insert_into_new_root(node * left, uint64_t key, node * right);
//node * start_new_root(uint64_t key, pointer * pointer);
void insert(key_t key, char * value);

// Deletion.

//int get_neighbor_index( node * n );
//node * adjust_root(node * root);
//node * coalesce_nodes(node * root, node * n, node * neighbor,
//                      int neighbor_index, uint64_t k_prime);
//node * redistribute_nodes(node * root, node * n, node * neighbor,
//                          int neighbor_index,
//        int k_prime_index, uint64_t k_prime);
//node * delete_entry( node * root, node * n, uint64_t key, void * pointer );
void delete(key_t key);

//void destroy_tree_nodes(node * root);
//node * destroy_tree(node * root);

#endif /* __BPT_H__*/
