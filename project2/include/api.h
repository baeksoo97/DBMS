#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "index.h"
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// FUNCTION PROTOTYPES.

// Open existing data file using 'pathname' or create one if not existed
int open_table(const char *pathname);

// Insert input 'key/value' (record) to data file at the right place
int db_insert(key_t key, char * value);

// Find the record containing input 'key'
int db_find(key_t key, char * ret_val);

// Find the matching record and delete it if found
int db_delete(key_t key);

void db_print_tree(void);

// Second message to the user.
void usage(void);

#endif /* __BPT_H__*/
