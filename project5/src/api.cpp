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
    if (!index_is_opened(table_id)){
        printf("ERROR DB_INSERT : table is not opened\n");
        return -1;
    }

    return insert(table_id, key, value);
}

// Find the matching record and delete it if found
int db_delete(int table_id, k_t key){
    if (!index_is_opened(table_id)){
        printf("ERROR DB_DELETE : table is not opened\n");
        return -1;
    }

    return delete_key(table_id, key);
}

// Find the record containing input 'key'
int db_find(int table_id, k_t key, char * ret_val, int trx_id){
    if (!index_is_opened(table_id)){
        printf("ERROR DB_FIND : table is not opened\n");
        return -1;
    }


    int ret = _find(table_id, key, ret_val);
    if (ret == 0)
        printf("find the record : key = %ld, value %s\n", key, ret_val);
    else
        printf("cannot find the record containing key %ld\n", key);

    return ret;
}

// Find the matching key and modify the values
int db_update(int table_id, k_t key, char * value, int trx_id){

}

// Allocate a transaction structure and initialize it
int trx_begin(void){

}

// Clean up the transaction with given trx_id and its related information
// that has been used in your lock manager.
int trx_commit(int trx_id){

}

// Write the pages relating to this table to disk and close the table
int close_table(int table_id){
    if (!index_is_opened(table_id)){
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
    if (!index_is_opened(table_id)){
        printf("ERROR DB_PRINT : table is not opened\n");
        return;
    }

    index_print_tree(table_id);
}

void db_print_table(void){
    index_print_table();
}

void db_print_buffer(void){
    index_print_buffer();
}

// Second message to the user.
void usage(void){
    printf("Enter any of the following commands after the prompt > \n"
           "\tb <num_buf>              -- Init DB with the size of <num_buf> (an integer).\n"
           "\to <data_file>            -- Open <data_file> (a string).\n"
           "\tc <table_id>             -- Close data_file having <table_id> (an integer).\n"
           "\ti <table_id> <k> <value> -- Insert <k> (an integer) <value> (a string) into <table_id> (an integer).\n"
           "\tI <table_id> <k1> <k2>   -- Insert <k1> ~ <k2> (an integer) into <table_id> (an integer).\n"
           "\tf <table_id> <k>         -- Find <table_id> (an integer) the value under key <k>.\n"
           "\td <table_id> <k>         -- Delete key <k> and its associated value from <table_id> (an integer).\n"
           "\tD <table_id> <k1> <k2>   -- Delete <k1> ~ <k2> (an integer) from <table_id> (an integer).\n"
           "\tp <table_id>             -- Print the B+ tree.\n"
           "\tt                        -- Print table id, path name, file descriptor table.\n"
           "\tn                        -- Print buffer.\n"
           "\ts                        -- Shutdown db.\n"
           "\tq                        -- Quit. (Or use Ctl-D.)\n"
           "\t?                        -- Print this help message.\n");
}