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

// Find the record containing input 'key'
int db_find(int table_id, k_t key, char * ret_val){
    if (!index_is_opened(table_id)){
        printf("ERROR DB_FIND : table is not opened\n");
        return -1;
    }

    int ret = _find(table_id, key, ret_val);
    if (ret == 0)
        printf("find the record : key = %lld, value %s\n", key, ret_val);
    else
        printf("cannot find the record containing key %lld\n", key);

    return ret;
}

// Find the matching record and delete it if found
int db_delete(int table_id, k_t key){
    if (!index_is_opened(table_id)){
        printf("ERROR DB_DELETE : table is not opened\n");
        return -1;
    }

    return delete_key(table_id, key);
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

    print_tree(table_id);
}

void db_print_table(){
    int i, fd;
    bool visit;
    printf("%10s %10s - %5s %5s\n", "table_id", "file_name", "fd", "is_open");
    for(i = 1; i <= 10; i++){
        visit = table_fd_map[i].first;
        fd = table_fd_map[i].second;
        string file_name;
        for(auto m : file_table_map){
            if (m.second == i){
                file_name = m.first;
            }
        }
        printf("%10d %10s - %5d %5d\n", i, file_name.c_str(), fd, visit);
    }
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