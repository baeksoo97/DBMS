#define Version "1.14"

#include "api.h"

// FUNCTION DEFINITIONS.

// Open existing data file using 'pathname' or create one if not existed
int open_table(const char * pathname){
    int fd;
    if (!access(pathname, F_OK)){
        // file exists
        fd = open(pathname, O_RDWR | O_SYNC, 0644); // open, read, write
        if (fd == 0){
            printf("Error : open_table\n");
            return -1;
        }
        printf("open already exist one : %d\n", fd);
        file_id = fd;
        file_init_header(1);
    }
    else{
        // file doesn't exist
        fd = open(pathname, O_CREAT | O_RDWR | O_SYNC, 0644); // create new, read, write
        if (fd == 0){
            printf("Error : open_table\n");
            return -1;
        }
        printf("create new one : %d\n", fd);
        file_id = fd;
        file_init_header(0);
    }

    return fd;
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
           "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
           "\tf <k>  -- Find the value under key <k>.\n"
           "\td <k>  -- Delete key <k> and its associated value.\n"
           "\tx -- Destroy the whole tree.  Start again with an empty tree of the "
           "same order.\n"
           "\tp -- Print the B+ tree.\n"
           "\tq -- Quit. (Or use Ctl-D.)\n"
           "\t? -- Print this help message.\n");
}