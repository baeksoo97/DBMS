#include "api.h"

// FUNCTION DEFINITIONS.

// Open existing data file using 'pathname' or create one if not existed
int open_table(char * pathname){
    int table_id = index_open_table(pathname);

    return table_id;
}

void close_table(char * pathname){
    if (FILE_ID < 0){
        if (strcmp(pathname, ""))
            printf("File is not opened\n");
        return;
    }

    index_close_table(pathname);
}

// Insert input 'key/value' (record) to data file at the right place
int db_insert(k_t key, char * value){
    if (FILE_ID < 0){
        printf("File is not opened\n");
        return -1;
    }

    return insert(key, value);
}

// Find the record containing input 'key'
int db_find(k_t key, char * ret_val){
    if (FILE_ID < 0){
        printf("File is not opened\n");
        return -1;
    }
    int ret = find(key, ret_val);
    if (ret == 0)
        printf("find the record : key = %ld, value %s\n", key, ret_val);
    else
        printf("cannot find the record containing key %ld\n", key);

    return ret;
}

// Find the matching record and delete it if found
int db_delete(k_t key){
    if (FILE_ID < 0){
        printf("File is not opened\n");
        return -1;
    }

    return delete_key(key);
}

void db_print_tree(void){
    if (FILE_ID < 0){
        printf("File is not opened\n");
        return;
    }

    print_tree();
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