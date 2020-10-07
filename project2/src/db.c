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
        file_init_header();
    }

    return fd;
}

//int close_table(const char * pathname, int fd){
    // close(fd);
//}

// Insertion

// Insert input 'key/value' (record) to data file at the right place
int db_insert(key_t key, char * value){
    record * pointer;

    // The current implementation ignores duplicates.
    if (db_find(key, value) == 0){
        return 0;
    }

    // Create a new record for the value.
    pointer = make_record(key, value);

    header_page = header();

    // Case: the tree does not exist yet. Start a new tree.
    if (header_page->h.root_pagenum == 0)
        return start_new_tree(key, pointer);

//    node * leaf;
//    // Case: the tree already exists. (Rest of function body.)
//    leaf = find_leaf(root, key, false);
//
//    // Case: leaf has room for key and pointer.
//    if (leaf->num_keys < order - 1) {
//        leaf = insert_into_leaf(leaf, key, pointer);
//        return root;
//    }
//
//    // Case: leaf must be split.
//    return insert_into_leaf_after_splitting(root, leaf, key, pointer);
}


/* Creates a new record to hold the value
 * to which a key refers.
 */
record * make_record(key_t key, char * value) {
    record * new_record = (record *)malloc(sizeof(record));

    if (new_record == NULL) {
        perror("Record creation.");
        exit(EXIT_FAILURE);
    }
    else {
        new_record->key = key;
        strcpy(new_record->value, value);
    }
    printf("new_record(%lld , %s)\n", new_record->key, new_record->value);
    return new_record;
}

/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
page_t * make_general_page(void) {
    page_t * new_page = make_page();

    new_page->g.parent = NULL;
    new_page->g.is_leaf = 0;
    new_page->g.num_keys = 0;
    new_page->g.next = NULL;
    return new_page;
}

page_t * make_internal_page(void){
    page_t * internal = make_general_page();

    internal->g.entry = (entry *)malloc(INTERNAL_ORDER  * sizeof(entry));
    if (internal->g.entry == NULL) {
        perror("New internal entry array.");
        exit(EXIT_FAILURE);
    }
    return internal;
}

page_t * make_leaf_page(void){
    page_t * leaf = make_general_page();

    leaf->g.record = (record *)malloc(LEAF_ORDER * sizeof(record));
    if (leaf->g.record == NULL) {
        perror("New leaf record array.");
        exit(EXIT_FAILURE);
    }
    leaf->g.is_leaf = 1;
    return leaf;
}

// First insertion: start a new tree.
int start_new_tree(key_t key, record * pointer) {
    page_t * root = make_leaf_page();

    root->g.parent = NULL;
    root->g.num_keys++;
    root->g.next = NULL;
    memcpy(root->g.record + 0, pointer, sizeof(record));

    file_init_root(root);

    printf("root %lld %s\n", root->g.record[0].key, root->g.record[0].value);
    return 0; // if success
    // return -1; if not success
}


// Search
// Find the record containing input 'key'
int db_find(key_t key, char * ret_val) {

    return 0;
}

// Find the matching record and delete it if found
int db_delete(key_t key){

}

void print_db(){
    printf("it's file");
}
