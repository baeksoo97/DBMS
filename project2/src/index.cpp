#include "index.h"

// FUNCTION DEFINITIONS.

// UTILITY.

int index_open_table(const char * pathname){
    int table_id;
    table_id = file_open(pathname);

    return table_id;
}

void index_close_table(const char * pathname){
    file_close(pathname);
}

// FIND.

/* Traces the path from the root to a leaf, searching by key.
 * Returns the pagenum containing the given key.
 */
pagenum_t find_leaf(key_t key){
    int i = 0;
    page_t * page;
    pagenum_t pagenum;

    header_page = header();
    if (header_page->h.root_pagenum == 0){
        return 0;
    }

    page = make_page();
    pagenum = header_page->h.root_pagenum;
    file_read_page(pagenum, page);
    while(!page->g.is_leaf){
        i = 0;
        while (i < page->g.num_keys){
            if (key >= page->g.entry[i].key) i++;
            else break;
        }

        if (i == 0)
            pagenum = page->g.next;
        else
            pagenum = page->g.entry[i - 1].pagenum;

        file_read_page(pagenum, page);
    }

    free_page(page);
    return pagenum;
}

// Master find function.
int find(key_t key, char * ret_val){
    int i;
    page_t * leaf;
    pagenum_t leaf_pagenum = find_leaf(key);
    if (leaf_pagenum == 0){
        return -1;
    }

    leaf = make_page();
    file_read_page(leaf_pagenum, leaf);
    for (i = 0; i < leaf->g.num_keys; i++){
        if (leaf->g.record[i].key == key) break;
    }
    if (i == leaf->g.num_keys){
        free_page(leaf);
        return -1;
    }
    else{
        free_page(leaf);
        strcpy(ret_val, leaf->g.record[i].value);
        return 0;
    }
}

// INSERTION.

/* Creates a new record to hold the value
 * to which a key refers.
 */
record * make_record(key_t key, char * value){
    record * new_record = (record *)malloc(sizeof(record));

    if (new_record == NULL){
        perror("ERROR MAKE_RECORD : cannot create record");
        exit(EXIT_FAILURE);
    }
    new_record->key = key;
    strcpy(new_record->value, value);

//    printf("new_record(%lld , %s)\n", new_record->key, new_record->value);
    return new_record;
}

/* Creates a new general page, which can be adapted
 * to serve as either a leaf or an internal page.
 */
page_t * make_general_page(void){
    page_t * new_page = make_page();

    new_page->g.parent = 0;
    new_page->g.is_leaf = 0;
    new_page->g.num_keys = 0;
    new_page->g.next = 0;
    return new_page;
}

// Creates a new internal by creating a page and then adapting it appropriately.
page_t * make_internal_page(void){
    page_t * internal = make_general_page();

    return internal;
}

// Creates a new leaf by creating a page and then adapting it appropriately.
page_t * make_leaf_page(void){
    page_t * leaf = make_general_page();

    leaf->g.is_leaf = 1;
    return leaf;
}

/* Finds the appropriate place to
 * split a page that is too big into two.
 */
int cut(int length){
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}

/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to
 * the page to the left of the key to be inserted.
 */
int get_left_index(page_t * parent, pagenum_t left_pagenum){
    int left_index = 0;

    if (parent->g.next == left_pagenum){
        left_index = -1;
    }
    else{
        while (left_index < parent->g.num_keys &&
               parent->g.entry[left_index].pagenum != left_pagenum)
            left_index++;
    }

    return left_index;
}

/* Inserts a new key and pointer to a page
 * into a page, causing the page's size to exceed
 * the order, and causing the page to split into two.
 */
int insert_into_page_after_splitting(pagenum_t old_pagenum, page_t * old_page, int left_index,
                                        key_t key, pagenum_t right_pagenum){
//    printf("insert_into_node_after_Splitting\n");
    int i, j, split;
    key_t k_prime;
    entry * temp_entry;
    page_t * new_page, * child;
    pagenum_t new_pagenum, child_pagenum;

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places.
     * Then create a new node and copy half of the
     * keys and pointers to the old page and
     * the other half to the new.
     */
    temp_entry = malloc((INTERNAL_ORDER + 1) * sizeof(entry));
    if (temp_entry == NULL){
        perror("ERROR INSERT_INTO_PAGE_AFTER_SPLITTING : cannot create temp_entry");
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < old_page->g.num_keys; i++, j++){
        if (j == left_index + 1) j++; // to store right page next to the left_index
        memcpy(temp_entry + j, old_page->g.entry + i, sizeof(entry));
    }

    temp_entry[left_index + 1].key = key;
    temp_entry[left_index + 1].pagenum = right_pagenum;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */
    split = cut(INTERNAL_ORDER - 1);

    new_page = make_internal_page();
    new_pagenum = file_alloc_page();
    old_page->g.num_keys = 0;
    for (i = 0; i < split; i++){
        memcpy(old_page->g.entry + i, temp_entry + i, sizeof(entry));
        old_page->g.num_keys++;
    }

    k_prime = temp_entry[split].key;
    new_page->g.next = temp_entry[split].pagenum;

    for (++i, j = 0; i < INTERNAL_ORDER + 1; i++, j++){
        memcpy(new_page->g.entry + j, temp_entry + i, sizeof(entry));
        new_page->g.num_keys++;
    }

    free(temp_entry);

    new_page->g.parent = old_page->g.parent;

    child = make_page();
    child_pagenum = new_page->g.next;
    file_read_page(child_pagenum, child);
    child->g.parent = new_pagenum;
    file_write_page(child_pagenum, child);
    for (i = 0; i < new_page->g.num_keys; i++){
        child_pagenum = new_page->g.entry[i].pagenum;
        file_read_page(child_pagenum, child);
        child->g.parent = new_pagenum;
        file_write_page(child_pagenum, child);
    }
    free_page(child);

    file_write_page(old_pagenum, old_page);
    file_write_page(new_pagenum, new_page);

    /* Insert a new key into the parent of the two
     * pages resulting from the split, with
     * the old page to the left and the new to the right.
     */
    return insert_into_parent(old_pagenum, old_page, k_prime, new_pagenum, new_page);
}

/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
int insert_into_page(pagenum_t n_pagenum, page_t * n_page,
                     int left_index, key_t key, pagenum_t right_pagenum){
    int i;
//    printf("insert_into_node\n");

    // Left page is left most.
    if (left_index == -1){
        for (i = n_page->g.num_keys; i > 0; i--){
            memcpy(n_page->g.entry + i, n_page->g.entry + (i - 1), sizeof(entry));
        }
    }
    else{
        for (i = n_page->g.num_keys; i > left_index; i--){
            memcpy(n_page->g.entry + i, n_page->g.entry + (i - 1), sizeof(entry));
        }
    }

    n_page->g.entry[left_index + 1].key = key;
    n_page->g.entry[left_index + 1].pagenum = right_pagenum;

    n_page->g.num_keys++;

    file_write_page(n_pagenum, n_page);

    free_page(n_page);

    return 0;
}

/* Inserts a new page (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
int insert_into_parent(pagenum_t left_pagenum, page_t * left, key_t key, pagenum_t right_pagenum, page_t * right){
//    printf("insert_into_parent\n");
    int left_index;
    page_t * parent;
    pagenum_t parent_pagenum = left->g.parent;

    // Case 1 : new root.
    if (parent_pagenum == 0)
        return insert_into_new_root(left_pagenum, left, key, right_pagenum, right);

    // Case 2 : leaf or page. (Remainder of function body.)

    // Find the parent's pointer to the left page.
    parent = make_page();
    file_read_page(parent_pagenum, parent);

    left_index = get_left_index(parent, left_pagenum);

    free_page(left);
    free_page(right);

    // Simple Case 2-1 : the new key fits into the page.
    if (parent->g.num_keys < INTERNAL_ORDER){
        return insert_into_page(parent_pagenum, parent, left_index, key, right_pagenum);
    }
    // Harder Case 2-2 : split a page in order to preserve the B+tree properties.
    return insert_into_page_after_splitting(parent_pagenum, parent, left_index, key, right_pagenum);
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
int insert_into_leaf_after_splitting(pagenum_t leaf_pagenum, page_t * leaf, key_t key, record * pointer){
//    printf("insert_into_leaf_after_splitting\n");
    record * temp_pointers;
    page_t * new_leaf;
    pagenum_t new_leaf_pagenum;
    key_t new_key;
    int insertion_index, split, i, j;

    temp_pointers = malloc((LEAF_ORDER + 1) * sizeof(record));
    if (temp_pointers == NULL){
        perror("ERROR INSERT_INTO_LEAF_AFTER_SPLITTING : cannot create temp_pointers");
        exit(EXIT_FAILURE);
    }

    new_leaf = make_leaf_page();
    new_leaf_pagenum = file_alloc_page();

    // Find the index of leaf records that key should be inserted
    insertion_index = 0;
    while (insertion_index < LEAF_ORDER && leaf->g.record[insertion_index].key < key)
        insertion_index++;

    // Copy original records to temporary records
    // (Make the insertion index be blank in temporary records)
    for (i = 0, j = 0; i < leaf->g.num_keys; i++, j++){
        if (j == insertion_index) j++;
        memcpy(temp_pointers + j, leaf->g.record + i, sizeof(record));
    }

    memcpy(temp_pointers + insertion_index, pointer, sizeof(record));

    // Split page that is too big into two
    split = cut(LEAF_ORDER);

    leaf->g.num_keys = 0;
    for (i = 0; i < split; i++){
        memcpy(leaf->g.record + i, temp_pointers + i, sizeof(record));
        leaf->g.num_keys++; // result : split
    }

    for (i = split, j = 0; i < LEAF_ORDER + 1; i++, j++){
        memcpy(new_leaf->g.record + j, temp_pointers + i, sizeof(record));
        new_leaf->g.num_keys++; // result :  LEAF_ORDER + 1 - split
    }

    free(temp_pointers);
    free(pointer);

    new_leaf->g.next = leaf->g.next;
    leaf->g.next = new_leaf_pagenum;

    new_leaf->g.parent = leaf->g.parent;
    new_key = new_leaf->g.record[0].key;

    file_write_page(leaf_pagenum, leaf);
    file_write_page(new_leaf_pagenum, new_leaf);

    return insert_into_parent(leaf_pagenum, leaf, new_key, new_leaf_pagenum, new_leaf);
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
int insert_into_leaf(pagenum_t leaf_pagenum, page_t * leaf, key_t key, record * pointer){
//    printf("insert_into_leaf\n");
    int i, insertion_point;

    insertion_point = 0;
    while (insertion_point < leaf->g.num_keys && leaf->g.record[insertion_point].key < key)
        insertion_point++;

    for (i = leaf->g.num_keys; i > insertion_point; i--){
        memcpy(leaf->g.record + i, leaf->g.record + (i - 1), sizeof(record));
    }
    memcpy(leaf->g.record + insertion_point, pointer, sizeof(record));
    leaf->g.num_keys++;

    file_write_page(leaf_pagenum, leaf);

    free_page(leaf);
    free(pointer);

    return 0;
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
int insert_into_new_root(pagenum_t left_pagenum, page_t * left, key_t key, pagenum_t right_pagenum, page_t * right){
//    printf("insert_into_new_root\n");
    page_t * root = make_internal_page();
    pagenum_t root_pagenum = file_alloc_page();

    root->g.parent = 0;
    root->g.is_leaf = 0;
    root->g.num_keys++;
    root->g.next = left_pagenum;
    root->g.entry[0].key = key;
    root->g.entry[0].pagenum = right_pagenum;

    left->g.parent = root_pagenum;
    right->g.parent = root_pagenum;

    header_page = header();
    header_page->h.root_pagenum = root_pagenum;

    file_write_page(root_pagenum, root);
    file_write_page(left_pagenum, left);
    file_write_page(right_pagenum, right);
    file_write_page(0, header_page);

    free_page(root);
    free_page(left);
    free_page(right);

    return 0;
}

// First insertion: start a new tree.
int insert_new_tree(key_t key, record * pointer){
    page_t * root = make_leaf_page();
    pagenum_t root_pagenum = file_alloc_page();

    root->g.parent = 0;
    root->g.num_keys++;
    root->g.next = 0;
    memcpy(root->g.record + 0, pointer, sizeof(record));

    header_page->h.root_pagenum = root_pagenum;

    file_write_page(root_pagenum, root);
    file_write_page(0, header_page);

    free_page(root);
    free(pointer);
    return 0;
}

// Master insertion function.
int insert(key_t key, char * value){
    record * pointer;
    pagenum_t leaf_pagenum;
    page_t * leaf;

    // The current implementation ignores duplicates.
    if (find(key, value) == 0){
        return -1;
    }

    // Create a new record for the value.
    pointer = make_record(key, value);

    header_page = header();

    // Case: the tree does not exist yet. Start a new tree.
    if (header_page->h.root_pagenum == 0){
        return insert_new_tree(key, pointer);
    }

    // Case: the tree already exists. (Rest of function body.)
    leaf_pagenum = find_leaf(key);
    if (leaf_pagenum == 0) {
        return -1;
    }

    leaf = make_page();
    file_read_page(leaf_pagenum, leaf);

    // Case: leaf has room for key and pointer.
    if (leaf->g.num_keys < LEAF_ORDER){
        return insert_into_leaf(leaf_pagenum, leaf, key, pointer);
    }

    // Case: leaf must be split.
    return insert_into_leaf_after_splitting(leaf_pagenum, leaf, key, pointer);
}


// DELETION.

/* Redistributes entries between two nodes when one has become
 * too small after deletion, but its neighbor is too big to append
 * the small node's entries without exceeding the maximum.
 */
int redistribute_nodes(pagenum_t parent_pagenum, page_t * parent,
                       pagenum_t n_pagenum, page_t * n_page,
                       pagenum_t neighbor_pagenum, page_t * neighbor,
                       int neighbor_index, int k_prime_index, key_t k_prime){
//    printf("redistribute\n");
    int i, j, split, neighbor_end;
    pagenum_t tmp_pagenum;

    /* Case: n has a neighbor to the left.
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */
    if (neighbor_index != -2){
        split = cut(neighbor->g.num_keys);
        if (!n_page->g.is_leaf){
            neighbor_end = neighbor->g.num_keys;

            for (i = split + 1, j = 0; i < neighbor_end; i++, j++){
                memcpy(n_page->g.entry + j, neighbor->g.entry + i, sizeof(entry));
                n_page->g.num_keys++;
                neighbor->g.num_keys--;
            }

            n_page->g.entry[j].key = k_prime;
            n_page->g.entry[j].pagenum = n_page->g.next;
            n_page->g.num_keys++;

            parent->g.entry[k_prime_index].key = neighbor->g.entry[split].key;
            n_page->g.next = neighbor->g.entry[split].pagenum;
            neighbor->g.num_keys--;

            page_t * tmp = make_page();
            tmp_pagenum = n_page->g.next;
            file_read_page(tmp_pagenum, tmp);
            tmp->g.parent = n_pagenum;
            file_write_page(tmp_pagenum, tmp);
            for (i = 0; i < n_page->g.num_keys; i++){
                tmp_pagenum = n_page->g.entry[i].pagenum;
                file_read_page(tmp_pagenum, tmp);
                tmp->g.parent = n_pagenum;
                file_write_page(tmp_pagenum, tmp);
            }
            free_page(tmp);
        }
        else{
            neighbor_end = neighbor->g.num_keys;

            for (i = split, j = 0; i < neighbor_end; i++, j++){
                memcpy(n_page->g.record + j, neighbor->g.record + i, sizeof(record));
                n_page->g.num_keys++;
                neighbor->g.num_keys--;
            }

            parent->g.entry[k_prime_index].key = n_page->g.record[0].key;
        }
    }
    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */
    else{
        split = cut(neighbor->g.num_keys);
        if (!n_page->g.is_leaf){
            n_page->g.entry[0].key = k_prime;
            n_page->g.entry[0].pagenum = neighbor->g.next;
            n_page->g.num_keys++;

            neighbor_end = neighbor->g.num_keys;
            for (i = 0, j = 1; i < split - 1; i++, j++){
                memcpy(n_page->g.entry + j, neighbor->g.entry + i, sizeof(entry));
                n_page->g.num_keys++;
                neighbor->g.num_keys--;
            }

            parent->g.entry[k_prime_index].key = neighbor->g.entry[split - 1].key;
            neighbor->g.next = neighbor->g.entry[split - 1].pagenum;
            neighbor->g.num_keys--;

            for (++i, j = 0; i < neighbor_end; i++, j++){
                memcpy(neighbor->g.entry + j, neighbor->g.entry + i, sizeof(entry));
            }

            page_t * tmp = make_page();
            for (i = 0; i < n_page->g.num_keys; i++){
                tmp_pagenum = n_page->g.entry[i].pagenum;
                file_read_page(tmp_pagenum, tmp);
                tmp->g.parent = n_pagenum;
                file_write_page(tmp_pagenum, tmp);
            }
            free_page(tmp);
        }
        else {
            neighbor_end = neighbor->g.num_keys;

            for (i = 0, j = 0; i < split; i++, j++){
                memcpy(n_page->g.record + j, neighbor->g.record + i, sizeof(record));
                n_page->g.num_keys++;
                neighbor->g.num_keys--;
            }
            for (i = split, j = 0; i < neighbor_end; i++, j++){
                memcpy(neighbor->g.record + j, neighbor->g.record + i, sizeof(record));
            }

            parent->g.entry[k_prime_index].key = neighbor->g.record[0].key;
        }
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    file_write_page(n_pagenum, n_page);
    file_write_page(neighbor_pagenum, neighbor);
    file_write_page(parent_pagenum, parent);

    free_page(n_page);
    free_page(neighbor);
    free_page(parent);

    return 0;
}

/* Coalesces a node that has become too small after deletion with a neighboring node that
 * can accept the additional entries without exceeding the maximum.
 */
int coalesce_nodes(pagenum_t parent_pagenum, page_t * parent, pagenum_t n_pagenum, page_t * n_page,
                      pagenum_t neighbor_pagenum, page_t * neighbor, int neighbor_index,
                      key_t k_prime){
//    printf("coalesce\n");
    int i, j, neighbor_insertion_index, n_end;
    page_t * tmp;
    pagenum_t tmp_pagenum;

    // Swap neighbor with node if node is on the extreme left and neighbor is to its right.
    if (neighbor_index == -2){
        tmp = n_page;
        n_page = neighbor;
        neighbor = tmp;

        tmp_pagenum = n_pagenum;
        n_pagenum = neighbor_pagenum;
        neighbor_pagenum = tmp_pagenum;
    }

    /* Starting point in the neighbor for copying keys and pointers from n.
     * Recall that n and neighbor have swapped places in the special case of
     * n being a leftmost child.
     */
    neighbor_insertion_index = neighbor->g.num_keys;

    /* Case: nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */
    if (!n_page->g.is_leaf){
        // Append k_prime.
        neighbor->g.entry[neighbor_insertion_index].key = k_prime;
        neighbor->g.entry[neighbor_insertion_index].pagenum = n_page->g.next;
        neighbor->g.num_keys++;

        n_end = n_page->g.num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++){
            memcpy(neighbor->g.entry + i, n_page->g.entry + j, sizeof(entry));
            neighbor->g.num_keys++;
            n_page->g.num_keys--;
        }

        // All children must now point up to the same parent.
        tmp = make_page();
        tmp_pagenum = neighbor->g.next;
        file_read_page(tmp_pagenum, tmp);
        tmp->g.parent = neighbor_pagenum;
        file_write_page(tmp_pagenum, tmp);
        for (i = 0; i < neighbor->g.num_keys; i++){
            tmp_pagenum = neighbor->g.entry[i].pagenum;
            file_read_page(tmp_pagenum, tmp);
            tmp->g.parent = neighbor_pagenum;
            file_write_page(tmp_pagenum, tmp);
        }
        free_page(tmp);
    }
    /* In a leaf, append the keys and pointers of n to the neighbor.
     * Set the neighbor's last pointer to point to what had been n's right neighbor.
     */
    else{
        n_end = n_page->g.num_keys;
        for (i = neighbor_insertion_index, j = 0; j < n_end; i++, j++){
            memcpy(neighbor->g.record + i, n_page->g.record + j, sizeof(record));
            neighbor->g.num_keys++;
            n_page->g.num_keys--;
        }
        neighbor->g.next = n_page->g.next;
    }

    file_write_page(neighbor_pagenum, neighbor);

    delete_entry(parent_pagenum, k_prime);

    file_free_page(n_pagenum);

    free_page(parent);
    free_page(n_page);
    free_page(neighbor);

    return 0;
}

/* Utility function for deletion.  Retrieves the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index(pagenum_t n_pagenum, page_t * n_page, pagenum_t parent_pagenum, page_t * parent){
    int i;

    /* Return the index of the key to the left of the pointer in the parent pointing
     * to n. If n is the leftmost child, this means return -1.
     */
    if (parent->g.next == n_pagenum){
        return -2; // neighbor : x / n_page : parent->g.next (leftmost)
    }

    for (i = 0; i < parent->g.num_keys; i++){
        if (parent->g.entry[i].pagenum == n_pagenum){
            if (i == 0) return -1; // neighbor : parent->g.next / n_page : parent->g.entry[0]
            else return i - 1;
        }
    }

    // Error state.
    printf("Search for nonexistent pointer to pointer in parent.\n");
    printf("Pointer:  %#lx\n", (unsigned long)n_page);
    exit(EXIT_FAILURE);
}

int adjust_root(pagenum_t root_pagenum){
    page_t * root;
    page_t * new_root;
    pagenum_t new_root_pagenum;

    root = make_page();
    file_read_page(root_pagenum, root);

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */
    if (root->g.num_keys > 0)
        return 0;

    /* Case: empty root.
     */
    new_root = make_page();
    // If it has a child, promote the first (only) child as the new root.
    if (!root->g.is_leaf){
        new_root_pagenum = root->g.next;
        file_read_page(new_root_pagenum, new_root);
        new_root->g.parent = 0;
        file_write_page(new_root_pagenum, new_root);

        header_page = header();
        header_page->h.root_pagenum = new_root_pagenum;
        file_write_page(0, header_page);
    }
    // If it is a leaf (has no children), then the whole tree is empty.
    else{
        header_page = header();
        header_page->h.root_pagenum = 0;
        file_write_page(0, header_page);
    }

    file_free_page(root_pagenum);
    free_page(root);
    free_page(new_root);

    return 0;
}

pagenum_t remove_entry_from_page(pagenum_t n_pagenum, page_t * n_page, key_t key){
    int i;

    i = 0;
    if (n_page->g.is_leaf){
        // Remove (key, value) and shift (key, value) accordingly.
        while (n_page->g.record[i].key != key)
            i++;
        for (++i; i < n_page->g.num_keys; i++)
            memcpy(n_page->g.record + (i - 1), n_page->g.record + i, sizeof(record));
    }
    else{
        // Remove (key, pagenum) and shift (key, pagenum) accordingly.
        while (n_page->g.entry[i].key != key)
            i++;
        for (++i; i < n_page->g.num_keys; i++)
            memcpy(n_page->g.entry + (i - 1), n_page->g.entry + i, sizeof(entry));
    }

    // One key fewer.
    n_page->g.num_keys--;

    file_write_page(n_pagenum, n_page);

    return n_pagenum;
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
int delete_entry(pagenum_t n_pagenum, key_t key){
    int min_keys, neighbor_index, k_prime_index, capacity;
    key_t k_prime;
    page_t * n_page;
    page_t * parent;
    page_t * neighbor;
    pagenum_t parent_pagenum, neighbor_pagenum;

    n_page = make_page();
    file_read_page(n_pagenum, n_page);

    // Remove key and pointer from node.
    n_pagenum = remove_entry_from_page(n_pagenum, n_page, key);

    /* Case:  deletion from the root.
     */
    header_page = header();
    if (header_page->h.root_pagenum == n_pagenum){
        free_page(n_page);
        return adjust_root(n_pagenum);
    }

    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Case:  node stays at or above minimum. (The simple case.)
     */
    min_keys = 1;
    if (n_page->g.num_keys >= min_keys)
        return 0;

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution is needed.
     */

    /* Find the appropriate neighbor node with which to coalesce.
     * Also find the key(k_prime) in the parent between the pointer to node n and the pointer
     * to the neighbor.
     */
    parent = make_page();
    parent_pagenum = n_page->g.parent;
    file_read_page(parent_pagenum, parent);

    neighbor_index = get_neighbor_index(n_pagenum, n_page, parent_pagenum, parent);
    k_prime_index = neighbor_index == -2 ? 0 : neighbor_index + 1;
    k_prime = parent->g.entry[k_prime_index].key;
    if (neighbor_index == -2) // n_page : parent->next (leftmost)
        neighbor_pagenum = parent->g.entry[0].pagenum;
    else if (neighbor_index == -1) // n_page : parent->g.entry[0]
        neighbor_pagenum = parent->g.next;
    else
        neighbor_pagenum = parent->g.entry[neighbor_index].pagenum;

    capacity = n_page->g.is_leaf == 1 ? LEAF_ORDER : INTERNAL_ORDER;

    neighbor = make_page();
    file_read_page(neighbor_pagenum, neighbor);

    /* Coalescence. */
    if ((!n_page->g.is_leaf && neighbor->g.num_keys + n_page->g.num_keys < capacity)
        || n_page->g.is_leaf)
        return coalesce_nodes(parent_pagenum, parent, n_pagenum, n_page, neighbor_pagenum,
                              neighbor, neighbor_index, k_prime);

    /* Redistribution. */
    else
        return redistribute_nodes(parent_pagenum, parent, n_pagenum, n_page, neighbor_pagenum,
                                  neighbor, neighbor_index, k_prime_index, k_prime);
}

// Master deletion function.
int delete(key_t key){
    pagenum_t key_leaf_pagenum;
    char * value;
    int key_found;

    value = malloc(120 * sizeof(char));
    key_found = find(key, value);
    key_leaf_pagenum = find_leaf(key);
    free(value);

    if (key_found != -1 && key_leaf_pagenum != 0){
        return delete_entry(key_leaf_pagenum, key);
    }
    else{
        return -1;
    }
}

// OUTPUT.

pagenum_t queue[QUEUE_SIZE];
int front = -1;
int rear = -1;
int q_size = 0;

int is_empty(void){
    if (front == rear) return 1;
    else return 0;
}

int is_full(void){
    if ((rear + 1) % QUEUE_SIZE == front) return 1;
    else return 0;
}

void enqueue(pagenum_t pagenum){
    if (!is_full()){
        rear = (rear + 1) % QUEUE_SIZE;
        queue[rear] = pagenum;
        q_size++;
    }
}

pagenum_t dequeue(void){
    if (is_empty()) return 0;
    else{
        q_size--;
        front = (front + 1) % QUEUE_SIZE;
        return queue[front];
    }
}

void print_tree(void){
    int i = 0;
    page_t * page = make_page();
    header_page = header();
    if (header_page->h.root_pagenum == 0){
        printf("Tree is empty\n");
        printf("----------\n");
        return;
    }
    enqueue(header_page->h.root_pagenum);

    while (!is_empty()){
        int temp_size = q_size;

        while (temp_size){
            pagenum_t pagenum = dequeue();
            printf("pagenum %lld ",pagenum);
            file_read_page(pagenum, page);

            if (page->g.is_leaf){
                printf("leaf : ");
                for (i = 0; i < page->g.num_keys; i++){
                    printf("(%lld, %s) ", page->g.record[i].key, page->g.record[i].value);
                }
                printf(" | ");
            }

            else {
                printf("internal : ");
                if (page->g.num_keys > 0){
                    printf("[%llu] ", page->g.next);
                    enqueue(page->g.next);
                }
                for (i = 0; i < page->g.num_keys; i++){
                    printf("%lld [%llu] ", page->g.entry[i].key, page->g.entry[i].pagenum);
                    enqueue(page->g.entry[i].pagenum);
                }
                printf(" | ");
            }

            temp_size--;
        }
        printf("\n");
    }

//    enqueue(header_page->h.root_pagenum);

    printf("----------\n");

    while (!is_empty()){
        int temp_size = q_size;

        while (temp_size){
            pagenum_t pagenum = dequeue();

            file_read_page(pagenum, page);

            if (page->g.is_leaf){
//                printf("leaf pagenum : %llu, parent : %llu, is_leaf : %d, num keys : %d, right sibling : %llu",
//                       pagenum, page->g.parent, page->g.is_leaf, page->g.num_keys, page->g.next);
//                printf(" | ");
            }
            else{
//                printf("internal pagenum : %llu, parent : %llu, is_leaf : %d, num keys : %d, one more : %llu",
//                       pagenum, page->g.parent, page->g.is_leaf, page->g.num_keys, page->g.next);

                enqueue(page->g.next);
                for (i = 0; i < page->g.num_keys; i++){
                    enqueue(page->g.entry[i].pagenum);
                }
                printf(" | ");
            }

            temp_size--;
        }
    }
    free_page(page);
}
