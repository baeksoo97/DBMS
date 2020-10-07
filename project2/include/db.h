#ifndef __DB_H__
#define __DB_H__

#include <stdio.h>
#include <unistd.h> // access function
#include <fcntl.h> // open
#include <string.h> // strcpy, memcpy
#include "file.h"

// Open existing data file using 'pathname' or create one if not existed
int open_table(const char *pathname);

// Insert input 'key/value' (record) to data file at the right place
int db_insert(key_t key, char * value);
record * make_record(key_t key, char * value);
page_t * make_general_page(void);
page_t * make_internal_page(void);
page_t * make_leaf_page(void);
int start_new_tree(key_t key, record * pointer);

// Find the record containing input 'key'
int db_find(key_t key, char * ret_val);

// Find the matching record and delete it if found
int db_delete(key_t key);

#endif //__DB_H__
