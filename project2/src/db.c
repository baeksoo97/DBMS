#include "db.h"

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
        file_init_pages();
    }

    return fd;
}

//int close_table(const char * pathname, int fd){
    // close(fd);
//}

// Insert input 'key/value' (record) to data file at the right place
int db_insert(key_t key, char * value){

}

// Find the record containing input 'key'
int db_find(key_t key, char * ret_val) {
}

// Find the matching record and delete it if found
int db_delete(key_t key){
}

void print_db(){
    printf("it's file");
}
