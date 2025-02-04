#ifndef __API_H__
#define __API_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include "index.h"
#include "transaction_manager.h"

// FUNCTION PROTOTYPES.

// Open existing data file using 'pathname' or create one if not existed
int open_table(char * pathname);

// Initialize buffer pool with given number and buffer manager
int init_db(int buf_num);

// Insert input 'key/value' (record) to data file at the right place
int db_insert(int table_id, int64_t key, char * value);

// Find the matching record and delete it if found
int db_delete(int table_id, int64_t key);

// Find the record containing input 'key'
int db_find(int table_id, int64_t key, char * ret_val, int trx_id);

// Find the matching key and modify the values
int db_update(int table_id, int64_t key, char * values, int trx_id);


// Write the pages relating to this table to disk and close the table
int close_table(int table_id);

// Flush all data from buffer and destroy allocated buffer
int shutdown_db(void);

// Print data file based on B+tree
void db_print_tree(int table_id);

void db_print_table(void);

void db_print_buffer(void);

// Second message to the user.
void print_usage(void);

#endif // __API_H__
