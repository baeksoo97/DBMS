#define Version "1.14"

#include "bpt.h"

// FUNCTION DEFINITIONS.

// Second message to the user.
void usage_2( void ) {
    printf("Enter any of the following commands after the prompt > :\n"
    "\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
    "\tf <k>  -- Find the value under key <k>.\n"
    "\tp <k> -- Print the path from the root to key k and its associated "
           "value.\n"
    "\tr <k1> <k2> -- Print the keys and values found in the range "
            "[<k1>, <k2>\n"
    "\td <k>  -- Delete key <k> and its associated value.\n"
    "\tx -- Destroy the whole tree.  Start again with an empty tree of the "
           "same order.\n"
    "\tt -- Print the B+ tree.\n"
    "\tl -- Print the keys of the leaves (bottom row of the tree).\n"
    "\tv -- Toggle output of pointer addresses (\"verbose\") in tree and "
           "leaves.\n"
    "\tq -- Quit. (Or use Ctl-D.)\n"
    "\t? -- Print this help message.\n");
}

// Master find function.
void find(key_t key) {

}

// Master insertion function.
void insert(key_t key, char * value ) {

}

// Master deletion function.
void delete(key_t key) {

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

