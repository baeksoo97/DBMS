#include "api.h"

// FUNCTION DEFINITIONS.

// Open existing data file using 'pathname' or create one if not existed
int open_table(const char * pathname){
    int table_id = index_open_table(pathname);
    return table_id;
}

//int close_table(const char * pathname, int fd){
// close(fd);
//}

// Insert input 'key/value' (record) to data file at the right place
int db_insert(key_t key, char * value){
    int ret = insert(key, value);
    return ret;
}

// Find the record containing input 'key'
int db_find(key_t key, char * ret_val){
    int ret = find(key, ret_val);
    return ret;
}

// Find the matching record and delete it if found
int db_delete(key_t key){
    int ret = delete(key);
    return ret;
}

void db_print_tree(void){
    print_tree();
}

// Second message to the user.
void usage(void){
    printf("Enter any of the following commands after the prompt > :\n"
           "\ti <k> <value> -- Insert <k> (an integer) <value> (a string).\n"
           "\tf <k>  -- Find the value under key <k>.\n"
           "\td <k>  -- Delete key <k> and its associated value.\n"
           "\tp -- Print the B+ tree.\n"
           "\tq -- Quit. (Or use Ctl-D.)\n"
           "\t? -- Print this help message.\n");
}