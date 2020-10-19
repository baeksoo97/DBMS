#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "file.h"

int buffer_open_table(const char * pathname);

int buffer_init_db(int num_buf);

int buffer_close_table(int table_id);

#endif //__BUFFER_H__
