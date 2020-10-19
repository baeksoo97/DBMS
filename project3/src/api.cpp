#include "api.h"

// FUNCTION DEFINITIONS.

// Open existing data file using 'pathname' or create one if not existed
int open_table(char * pathname){
    int table_id = index_open_table(pathname);

    return table_id;
}

// Initialize buffer pool with given number and buffer manager
int init_db(int buf_num){
    return index_init_db(buf_num);
}

// Insert input 'key/value' (record) to data file at the right place
int db_insert(int table_id, k_t key, char * value){
    if (!is_table_opened(table_id)) {
        printf("ERROR DB_INSERT : table is not opened\n");
        return -1;
    }

    return insert(table_id, key, value);
}

// Find the record containing input 'key'
int db_find(int table_id, k_t key, char * ret_val){
    if (!is_table_opened(table_id)) {
        printf("ERROR DB_FIND : table is not opened\n");
        return -1;
    }

    int ret = find(table_id, key, ret_val);
    if (ret == 0)
        printf("find the record : key = %lld, value %s\n", key, ret_val);
    else
        printf("cannot find the record containing key %lld\n", key);

    return ret;
}

// Find the matching record and delete it if found
int db_delete(int table_id, k_t key){
    if (!is_table_opened(table_id)) {
        printf("ERROR DB_DELETE : table is not opened\n");
        return -1;
    }

    return delete_key(table_id, key);
}

// Write the pages relating to this table to disk and close the table
int close_table(int table_id){
    if (!is_table_opened(table_id)) {
        printf("ERROR DB_CLOSE : table is not opened\n");
        return -1;
    }

    return index_close_table(table_id);
}

// Flush all data from buffer and destroy allocated buffer
int shutdown_db(void){
    return index_close_table();
}

// Print data file based on B+tree
void db_print_tree(int table_id){
    if (!is_table_opened(table_id)) {
        printf("ERROR DB_PRINT : table is not opened\n");
        return;
    }

    print_tree(table_id);
}

// Second message to the user.
void usage(void){
    printf("Enter any of the following commands after the prompt > \n"
           "\to <data_file> -- Open <data_file> (a string).\n"
           "\tc <data_file> -- Close <data_file> (a string).\n"
           "\ti <k> <value> -- Insert <k> (an integer) <value> (a string).\n"
           "\tf <k>         -- Find the value under key <k>.\n"
           "\td <k>         -- Delete key <k> and its associated value.\n"
           "\tp             -- Print the B+ tree.\n"
           "\tq             -- Quit. (Or use Ctl-D.)\n"
           "\t?             -- Print this help message.\n");
}