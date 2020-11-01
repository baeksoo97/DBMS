#include "index.h"

// FUNCTION DEFINITIONS.

// UTILITY.

int index_open_table(const char * pathname){
    return buffer_open_table(pathname);
}

int index_init_db(int buf_num){
    return buffer_init_db(buf_num);
}

int index_close_table(int table_id){
    return buffer_close_table(table_id);
}

int index_is_opened(int table_id){
    return buffer_is_opened(table_id);
}
// FIND.

/* Traces the path from the root to a leaf, searching by key.
 * Returns the pagenum containing the given key.
 */
pagenum_t find_leaf(int table_id, pagenum_t root_pagenum, k_t key){
    int i;
    page_t * page;
    pagenum_t pagenum, prev_pagenum;

    if (root_pagenum == 0){
        return 0;
    }

    page = make_page();
    pagenum = root_pagenum;
    prev_pagenum = pagenum;

    buffer_read_page(table_id, pagenum, page);
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

        buffer_unpin_frame(table_id, prev_pagenum);

        buffer_read_page(table_id, pagenum, page);
        prev_pagenum = pagenum;
    }

    buffer_unpin_frame(table_id, pagenum);

    free_page(page);

    printf("                                                                                                               file_leaf\n");
    return pagenum;
}

// Master find function.
int find(int table_id, pagenum_t root_pagenum, k_t key, char * ret_val){
    printf("                                                                                                               find\n");
    int i;
    page_t * leaf;
    pagenum_t leaf_pagenum = find_leaf(table_id, root_pagenum, key);
    if (leaf_pagenum == 0){
        return -1;
    }

    leaf = make_page();
    buffer_read_page(table_id, leaf_pagenum, leaf);
    for (i = 0; i < leaf->g.num_keys; i++){
        if (leaf->g.record[i].key == key) break;
    }
    if (i == leaf->g.num_keys){
        buffer_unpin_frame(table_id, leaf_pagenum);
        free_page(leaf);
        return -1;
    }
    else{
        strcpy(ret_val, leaf->g.record[i].value);
        buffer_unpin_frame(table_id, leaf_pagenum);
        free_page(leaf);
        return 0;
    }
}

int _find(int table_id, k_t key, char * ret_val){
    page_t * header_page;
    pagenum_t root_pagenum;

    header_page = buffer_read_header(table_id);
    root_pagenum = header_page->h.root_pagenum;
    buffer_unpin_frame(table_id, 0);
    free_page(header_page);

    return find(table_id, root_pagenum, key, ret_val);
}

// INSERTION.

/* Creates a new record to hold the value
 * to which a key refers.
 */
struct record * make_record(k_t key, char * value){
    struct record * new_record = (struct record *)malloc(sizeof(struct record));

    if (new_record == NULL){
        perror("ERROR MAKE_RECORD");
        exit(EXIT_FAILURE);
    }
    new_record->key = key;
    strcpy(new_record->value, value);

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
int insert_into_page_after_splitting(int table_id, pagenum_t old_pagenum, page_t * old_page, int left_index,
                                     k_t key, pagenum_t right_pagenum){
//    printf("insert_into_node_after_Splitting\n");
    int i, j, split;
    k_t k_prime;
    struct entry * temp_entry;
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
    temp_entry = (struct entry *)malloc((INTERNAL_ORDER + 1) * sizeof(struct entry));
    if (temp_entry == NULL){
        perror("ERROR INSERT_INTO_PAGE_AFTER_SPLITTING");
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < old_page->g.num_keys; i++, j++){
        if (j == left_index + 1) j++; // to store right page next to the left_index
        memcpy(temp_entry + j, old_page->g.entry + i, sizeof(struct entry));
    }

    temp_entry[left_index + 1].key = key;
    temp_entry[left_index + 1].pagenum = right_pagenum;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */
    split = cut(INTERNAL_ORDER - 1);

    new_page = make_internal_page();
    new_pagenum = buffer_alloc_page(table_id);
    old_page->g.num_keys = 0;
    for (i = 0; i < split; i++){
        memcpy(old_page->g.entry + i, temp_entry + i, sizeof(struct entry));
        old_page->g.num_keys++;
    }

    k_prime = temp_entry[split].key;
    new_page->g.next = temp_entry[split].pagenum;

    for (++i, j = 0; i < INTERNAL_ORDER + 1; i++, j++){
        memcpy(new_page->g.entry + j, temp_entry + i, sizeof(struct entry));
        new_page->g.num_keys++;
    }

    free(temp_entry);

    new_page->g.parent = old_page->g.parent;

    buffer_write_page(table_id, old_pagenum, old_page);
    buffer_unpin_frame(table_id, old_pagenum, 2);

    // update child
    child = make_page();
    child_pagenum = new_page->g.next;
    buffer_read_page(table_id, child_pagenum, child);
    child->g.parent = new_pagenum;
    buffer_write_page(table_id, child_pagenum, child);
    buffer_unpin_frame(table_id, child_pagenum, 2);

    for (i = 0; i < new_page->g.num_keys; i++){
        child_pagenum = new_page->g.entry[i].pagenum;
        buffer_read_page(table_id, child_pagenum, child);
        child->g.parent = new_pagenum;
        buffer_write_page(table_id, child_pagenum, child);
        buffer_unpin_frame(table_id, child_pagenum, 2);
    }
    free_page(child);

    buffer_write_page(table_id, new_pagenum, new_page);
    buffer_unpin_frame(table_id, new_pagenum);

    /* Insert a new key into the parent of the two
     * pages resulting from the split, with
     * the old page to the left and the new to the right.
     */
    return insert_into_parent(table_id, old_pagenum, old_page, k_prime, new_pagenum, new_page);
}

/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
int insert_into_page(int table_id, pagenum_t n_pagenum, page_t * n_page,
                     int left_index, k_t key, pagenum_t right_pagenum){
    int i;
//    printf("insert_into_node\n");

    // Left page is left most.
    if (left_index == -1){
        for (i = n_page->g.num_keys; i > 0; i--){
            memcpy(n_page->g.entry + i, n_page->g.entry + (i - 1), sizeof(struct entry));
        }
    }
    else{
        for (i = n_page->g.num_keys; i > left_index; i--){
            memcpy(n_page->g.entry + i, n_page->g.entry + (i - 1), sizeof(struct entry));
        }
    }

    n_page->g.entry[left_index + 1].key = key;
    n_page->g.entry[left_index + 1].pagenum = right_pagenum;

    n_page->g.num_keys++;

    buffer_write_page(table_id, n_pagenum, n_page);
    buffer_unpin_frame(table_id, n_pagenum, 2);
    free_page(n_page);

    return 0;
}

/* Inserts a new page (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
int insert_into_parent(int table_id, pagenum_t left_pagenum, page_t * left, k_t key,
                       pagenum_t right_pagenum, page_t * right){
    int left_index;
    page_t * parent;
    pagenum_t parent_pagenum = left->g.parent;

    // Case 1 : new root.
    if (parent_pagenum == 0)
        return insert_into_new_root(table_id, left_pagenum, left, key, right_pagenum, right);

    // Case 2 : leaf or page. (Remainder of function body.)

    // Find the parent's pointer to the left page.
    parent = make_page();
    buffer_read_page(table_id, parent_pagenum, parent);

    left_index = get_left_index(parent, left_pagenum);

    free_page(left);
    free_page(right);

    // Simple Case 2-1 : the new key fits into the page.
    if (parent->g.num_keys < INTERNAL_ORDER){
        return insert_into_page(table_id, parent_pagenum, parent, left_index, key, right_pagenum);
    }
    // Harder Case 2-2 : split a page in order to preserve the B+tree properties.
    return insert_into_page_after_splitting(table_id, parent_pagenum, parent, left_index, key, right_pagenum);
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
int insert_into_leaf_after_splitting(int table_id, pagenum_t leaf_pagenum, page_t * leaf, k_t key, struct record * pointer){
    struct record * temp_pointers;
    page_t * new_leaf;
    pagenum_t new_leaf_pagenum;
    k_t new_key;
    int insertion_index, split, i, j;

    temp_pointers = (struct record *)malloc((LEAF_ORDER + 1) * sizeof(struct record));
    if (temp_pointers == NULL){
        perror("ERROR INSERT_INTO_LEAF_AFTER_SPLITTING");
        exit(EXIT_FAILURE);
    }

    new_leaf = make_leaf_page();
    new_leaf_pagenum = buffer_alloc_page(table_id);

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

    buffer_write_page(table_id, leaf_pagenum, leaf);
    buffer_write_page(table_id, new_leaf_pagenum, new_leaf);

    buffer_unpin_frame(table_id, leaf_pagenum, 2);
    buffer_unpin_frame(table_id, new_leaf_pagenum);

    return insert_into_parent(table_id, leaf_pagenum, leaf, new_key, new_leaf_pagenum, new_leaf);
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
int insert_into_leaf(int table_id, pagenum_t leaf_pagenum, page_t * leaf, k_t key, record * pointer){
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

    buffer_write_page(table_id, leaf_pagenum, leaf);
    buffer_unpin_frame(table_id, leaf_pagenum, 2);

    free_page(leaf);
    free(pointer);

    return 0;
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
int insert_into_new_root(int table_id, pagenum_t left_pagenum, page_t * left, k_t key, pagenum_t right_pagenum, page_t * right){
//    printf("insert_into_new_root\n");
    page_t * root, * header_page;
    pagenum_t root_pagenum;

    // update root
    root = make_internal_page();
    root_pagenum = buffer_alloc_page(table_id);

    root->g.parent = 0;
    root->g.is_leaf = 0;
    root->g.num_keys++;
    root->g.next = left_pagenum;
    root->g.entry[0].key = key;
    root->g.entry[0].pagenum = right_pagenum;

    buffer_write_page(table_id, root_pagenum, root);
    buffer_unpin_frame(table_id, root_pagenum);
    free_page(root);

    // update left
    left->g.parent = root_pagenum;
    buffer_write_page(table_id, left_pagenum, left);
    buffer_unpin_frame(table_id, left_pagenum, 1);
    free_page(left);

    // update right
    right->g.parent = root_pagenum;
    buffer_write_page(table_id, right_pagenum, right);
    buffer_unpin_frame(table_id, right_pagenum, 1);
    free_page(right);

    // update header
    header_page = buffer_read_header(table_id);
    header_page->h.root_pagenum = root_pagenum;

    buffer_write_page(table_id, 0, header_page);
    buffer_unpin_frame(table_id, 0, 2);
    free_page(header_page);

    return 0;
}

// First insertion: start a new tree.
int insert_new_tree(int table_id, k_t key, record * pointer){
    page_t * root, * header_page;
    pagenum_t root_pagenum;

    // update root
    root = make_leaf_page();
    root_pagenum = buffer_alloc_page(table_id);

    root->g.parent = 0;
    root->g.num_keys++;
    root->g.next = 0;
    memcpy(root->g.record + 0, pointer, sizeof(record));

    buffer_write_page(table_id, root_pagenum, root);
    buffer_unpin_frame(table_id, root_pagenum);
    free_page(root);

    // update header
    header_page = buffer_read_header(table_id);
    header_page->h.root_pagenum = root_pagenum;

    buffer_write_page(table_id, 0, header_page);
    buffer_unpin_frame(table_id, 0, 2);
    free_page(header_page);

    free(pointer);
    printf("                                                                                                               insert_new_tree\n");

    return 0;
}

// Master insertion function.
int insert(int table_id, k_t key, char * value){
    record * pointer;
    page_t * leaf, * header_page;
    pagenum_t leaf_pagenum, root_pagenum;

    header_page = buffer_read_header(table_id);
    root_pagenum = header_page->h.root_pagenum;
    free_page(header_page);
    buffer_unpin_frame(table_id, 0);

    // The current implementation ignores duplicates.
    if (find(table_id, root_pagenum, key, value) == 0){
        return -1;
    }

    // Create a new record for the value.
    pointer = make_record(key, value);

    // Case: the tree does not exist yet. Start a new tree.
    if (root_pagenum == 0){
        return insert_new_tree(table_id, key, pointer);
    }

    // Case: the tree already exists. (Rest of function body.)
    leaf_pagenum = find_leaf(table_id, root_pagenum, key);
    if (leaf_pagenum == 0) {
        return -1;
    }

    leaf = make_page();
    buffer_read_page(table_id, leaf_pagenum, leaf);

    // Case: leaf has room for key and pointer.
    if (leaf->g.num_keys < LEAF_ORDER){
        return insert_into_leaf(table_id, leaf_pagenum, leaf, key, pointer);
    }

    // Case: leaf must be split.
    return insert_into_leaf_after_splitting(table_id, leaf_pagenum, leaf, key, pointer);
}


// DELETION.

/* Redistributes entries between two nodes when one has become
 * too small after deletion, but its neighbor is too big to append
 * the small node's entries without exceeding the maximum.
 */
int redistribute_nodes(int table_id, pagenum_t parent_pagenum, page_t * parent,
                       pagenum_t n_pagenum, page_t * n_page,
                       pagenum_t neighbor_pagenum, page_t * neighbor,
                       int neighbor_index, int k_prime_index, k_t k_prime){
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
                memcpy(n_page->g.entry + j, neighbor->g.entry + i, sizeof(struct entry));
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
            buffer_read_page(table_id, tmp_pagenum, tmp);
            tmp->g.parent = n_pagenum;
            buffer_write_page(table_id, tmp_pagenum, tmp);
            buffer_unpin_frame(table_id, tmp_pagenum, 2);

            for (i = 0; i < n_page->g.num_keys; i++){
                tmp_pagenum = n_page->g.entry[i].pagenum;
                buffer_read_page(table_id, tmp_pagenum, tmp);
                tmp->g.parent = n_pagenum;
                buffer_write_page(table_id, tmp_pagenum, tmp);
                buffer_unpin_frame(table_id, tmp_pagenum, 2);
            }
            free_page(tmp);
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
                memcpy(n_page->g.entry + j, neighbor->g.entry + i, sizeof(struct entry));
                n_page->g.num_keys++;
                neighbor->g.num_keys--;
            }

            parent->g.entry[k_prime_index].key = neighbor->g.entry[split - 1].key;
            neighbor->g.next = neighbor->g.entry[split - 1].pagenum;
            neighbor->g.num_keys--;

            for (++i, j = 0; i < neighbor_end; i++, j++){
                memcpy(neighbor->g.entry + j, neighbor->g.entry + i, sizeof(struct entry));
            }

            page_t * tmp = make_page();
            for (i = 0; i < n_page->g.num_keys; i++){
                tmp_pagenum = n_page->g.entry[i].pagenum;
                buffer_read_page(table_id, tmp_pagenum, tmp);
                tmp->g.parent = n_pagenum;
                buffer_write_page(table_id, tmp_pagenum, tmp);
                buffer_unpin_frame(table_id, tmp_pagenum, 2);
            }
            free_page(tmp);
        }
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    buffer_write_page(table_id, n_pagenum, n_page);
    buffer_write_page(table_id, neighbor_pagenum, neighbor);
    buffer_write_page(table_id, parent_pagenum, parent);

    buffer_unpin_frame(table_id, n_pagenum, 2);
    buffer_unpin_frame(table_id, neighbor_pagenum, 2);
    buffer_unpin_frame(table_id, parent_pagenum, 2);

    free_page(n_page);
    free_page(neighbor);
    free_page(parent);

    printf("                                                                                                               redistribute\n");

    return 0;
}

/* Coalesces a node that has become too small after deletion with a neighboring node that
 * can accept the additional entries without exceeding the maximum.
 */
int coalesce_nodes(int table_id, pagenum_t parent_pagenum, page_t * parent, pagenum_t n_pagenum, page_t * n_page,
                   pagenum_t neighbor_pagenum, page_t * neighbor, int neighbor_index,
                   k_t k_prime){
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
            memcpy(neighbor->g.entry + i, n_page->g.entry + j, sizeof(struct entry));
            neighbor->g.num_keys++;
            n_page->g.num_keys--;
        }

        // All children must now point up to the same parent.
        // update child
        tmp = make_page();
        tmp_pagenum = neighbor->g.next;
        buffer_read_page(table_id, tmp_pagenum, tmp);
        tmp->g.parent = neighbor_pagenum;
        buffer_write_page(table_id, tmp_pagenum, tmp);
        buffer_unpin_frame(table_id, tmp_pagenum, 2);

        for (i = 0; i < neighbor->g.num_keys; i++){
            tmp_pagenum = neighbor->g.entry[i].pagenum;
            buffer_read_page(table_id, tmp_pagenum, tmp);
            tmp->g.parent = neighbor_pagenum;
            buffer_write_page(table_id, tmp_pagenum, tmp);
            buffer_unpin_frame(table_id, tmp_pagenum, 2);
        }
        free_page(tmp);
    }
    /* In a leaf, append the keys and pointers of n to the neighbor.
     * Set the neighbor's last pointer to point to what had been n's right neighbor.
     */
    else{
        n_end = n_page->g.num_keys;
        for (i = neighbor_insertion_index, j = 0; j < n_end; i++, j++){
            memcpy(neighbor->g.record + i, n_page->g.record + j, sizeof(struct record));
            neighbor->g.num_keys++;
            n_page->g.num_keys--;
        }
        neighbor->g.next = n_page->g.next;
    }

    buffer_write_page(table_id, neighbor_pagenum, neighbor);

    buffer_unpin_frame(table_id, n_pagenum);
    buffer_unpin_frame(table_id, neighbor_pagenum, 2);
    buffer_unpin_frame(table_id, parent_pagenum);

    delete_entry(table_id, parent_pagenum, k_prime);

    buffer_free_page(table_id, n_pagenum);

    printf("                                                                                                               coalesce\n");


    free_page(n_page);
    free_page(neighbor);
    free_page(parent);

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

int adjust_root(int table_id, pagenum_t root_pagenum){
    page_t * root, * new_root, * header_page;
    pagenum_t new_root_pagenum;

    printf("                                                                                                               adjust_root\n");

    root = make_page();
    buffer_read_page(table_id, root_pagenum, root);

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */
    if (root->g.num_keys > 0){
        buffer_unpin_frame(table_id, root_pagenum);
        free_page(root);
        return 0;
    }

    /* Case: empty root.
     */
    new_root = make_page();
    header_page = buffer_read_header(table_id);

    // If it has a child, promote the first (only) child as the new root.
    if (!root->g.is_leaf){
        new_root_pagenum = root->g.next;
        buffer_read_page(table_id, new_root_pagenum, new_root);
        new_root->g.parent = 0;
        buffer_write_page(table_id, new_root_pagenum, new_root);

        buffer_unpin_frame(table_id, new_root_pagenum, 2);

        header_page->h.root_pagenum = new_root_pagenum;
        buffer_write_page(table_id, 0, header_page);
        buffer_unpin_frame(table_id, 0, 2);
    }
    // If it is a leaf (has no children), then the whole tree is empty.
    else{
        header_page->h.root_pagenum = 0;
        buffer_write_page(table_id, 0, header_page);
        buffer_unpin_frame(table_id, 0, 2);
    }

    buffer_free_page(table_id, root_pagenum);
    buffer_unpin_frame(table_id, root_pagenum);

    free_page(root);
    free_page(new_root);
    free_page(header_page);

    return 0;
}

pagenum_t remove_entry_from_page(int table_id, pagenum_t n_pagenum, page_t * n_page, k_t key){
    int i;

    i = 0;
    if (n_page->g.is_leaf){
        // Remove (key, value) and shift (key, value) accordingly.
        while (n_page->g.record[i].key != key)
            i++;
        for (++i; i < n_page->g.num_keys; i++)
            memcpy(n_page->g.record + (i - 1), n_page->g.record + i, sizeof(struct record));
    }
    else{
        // Remove (key, pagenum) and shift (key, pagenum) accordingly.
        while (n_page->g.entry[i].key != key)
            i++;
        for (++i; i < n_page->g.num_keys; i++)
            memcpy(n_page->g.entry + (i - 1), n_page->g.entry + i, sizeof(struct entry));
    }

    // One key fewer.
    n_page->g.num_keys--;

    buffer_write_page(table_id, n_pagenum, n_page);
    buffer_unpin_frame(table_id, n_pagenum);

    printf("                                                                                                               remove_entry_From+page\n");

    return n_pagenum;
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
int delete_entry(int table_id, pagenum_t n_pagenum, k_t key){
    int min_keys, neighbor_index, k_prime_index, capacity;
    k_t k_prime;
    page_t * n_page, * parent, * neighbor;
    pagenum_t parent_pagenum, neighbor_pagenum;
    printf("                                                                                                               delete_entry\n");


    n_page = make_page();
    buffer_read_page(table_id, n_pagenum, n_page);

    // Remove key and pointer from node.
    n_pagenum = remove_entry_from_page(table_id, n_pagenum, n_page, key);

    /* Case:  deletion from the root.
     */
    if (n_page->g.parent == 0){
        free_page(n_page);
        buffer_unpin_frame(table_id, n_pagenum);
        return adjust_root(table_id, n_pagenum);
    }

    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Case:  node stays at or above minimum. (The simple case.)
     */
    min_keys = 1;
    if (n_page->g.num_keys >= min_keys){
        free_page(n_page);
        buffer_unpin_frame(table_id, n_pagenum);
        return 0;
    }

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution is needed.
     */

    /* Find the appropriate neighbor node with which to coalesce.
     * Also find the key(k_prime) in the parent between the pointer to node n and the pointer
     * to the neighbor.
     */
    parent = make_page();
    parent_pagenum = n_page->g.parent;
    buffer_read_page(table_id, parent_pagenum, parent);

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
    buffer_read_page(table_id, neighbor_pagenum, neighbor);

    /* Coalescence. */
    if ((!n_page->g.is_leaf && neighbor->g.num_keys + n_page->g.num_keys < capacity)
        || n_page->g.is_leaf)
        return coalesce_nodes(table_id, parent_pagenum, parent, n_pagenum, n_page, neighbor_pagenum,
                              neighbor, neighbor_index, k_prime);

    /* Redistribution. */
    else
        return redistribute_nodes(table_id, parent_pagenum, parent, n_pagenum, n_page, neighbor_pagenum,
                                  neighbor, neighbor_index, k_prime_index, k_prime);
}

// Master deletion function.
int delete_key(int table_id, k_t key){
    page_t * header_page;
    pagenum_t key_leaf_pagenum, root_pagenum;
    char * value;
    int key_found;

    value = (char *)malloc(120 * sizeof(char));
    header_page = buffer_read_header(table_id);
    root_pagenum = header_page->h.root_pagenum;

    key_found = find(table_id, root_pagenum, key, value);
    key_leaf_pagenum = find_leaf(table_id, root_pagenum, key);

    buffer_unpin_frame(table_id, 0);
    free(header_page);
    free(value);

    printf("                                                                                                               delete_key\n");

    if (key_found != -1 && key_leaf_pagenum != 0){
        return delete_entry(table_id, key_leaf_pagenum, key);
    }
    else{
        return -1;
    }
}

// OUTPUT.

void index_print_tree(int table_id, bool verbose){
    buffer_print_tree(table_id, verbose);
}

void index_print_table(void){
    buffer_print_table();
}

void index_print_buffer(void){
    buffer_print();
}