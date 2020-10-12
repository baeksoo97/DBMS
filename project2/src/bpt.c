#define Version "1.14"

#include "bpt.h"

// FUNCTION DEFINITIONS.

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

void print_tree(void){
    db_print_tree();
}

// Master find function.
void find(key_t key, bool verbose){
    char value[120];
    if (db_find(key, value) == 0)
        printf("find key %lld -> value %s\n", key, value);
    else
        printf("find not key %lld\n", key);
    if (verbose) db_print_tree();
}

// Master insertion function.
void insert(key_t key, char * value, bool verbose){
    db_insert(key, value);
    if (verbose) db_print_tree();
}

// Master deletion function.
void delete(key_t key, bool verbose) {
    db_delete(key);
    if (verbose) db_print_tree();
}

//void destroy_tree_nodes(node * root) {
//    int i;
//    if (root->is_leaf)
//        for (i = 0; i < root->num_keys; i++)
//            free(root->pointers[i]);
//    else
//        for (i = 0; i < root->num_keys + 1; i++)
//            destroy_tree_nodes(root->pointers[i]);
//    free(root->pointers);
//    free(root->keys);
//    free(root);
//}
//
//
//node * destroy_tree(node * root) {
//    destroy_tree_nodes(root);
//    return NULL;
//}
