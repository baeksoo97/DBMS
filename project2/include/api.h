#ifndef __API_H__
#define __API_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include "index.h"

// FUNCTION PROTOTYPES.

// Open existing data file using 'pathname' or create one if not existed
int open_table(char * pathname);

// Close existing data file using 'pathname'
void close_table(char * pathname = (char *)"");

// Insert input 'key/value' (record) to data file at the right place
int db_insert(k_t key, char * value);

// Find the record containing input 'key'
int db_find(k_t key, char * ret_val);

// Find the matching record and delete it if found
int db_delete(k_t key);

void db_print_tree(void);

// Second message to the user.
void usage(void);

#endif // __API_H__
