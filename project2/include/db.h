#ifndef __DB_H__
#define __DB_H__

#include <stdio.h>
#include <unistd.h> // access function
#include <fcntl.h>
#include "file.h"

// Open existing data file using 'pathname' or create one if not existed
int open_table(const char *pathname);

// Insert input 'key/value' (record) to data file at the right place
int db_insert(key_t key, char * value);

// Find the record containing input 'key'
int db_find(key_t key, char * ret_val);

// Find the matching record and delete it if found
int db_delete(key_t key);

#endif //__DB_H__
