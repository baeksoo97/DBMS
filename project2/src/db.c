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

// Insertion

// Insert input 'key/value' (record) to data file at the right place
int db_insert(key_t key, char * value){
    record * pointer;
    pagenum_t leaf_pagenum;
    page_t * leaf;

    // The current implementation ignores duplicates.
    if (db_find(key, value) == 0){
        printf("value find %s\n", value);
        return -1;
    }

    // Create a new record for the value.
    pointer = make_record(key, value);

    header_page = header();

    // Case: the tree does not exist yet. Start a new tree.
    if (header_page->h.root_pagenum == 0) {
        return db_insert_new_tree(key, pointer);
    }

    // Case: the tree already exists. (Rest of function body.)
    leaf_pagenum = db_find_leaf(key);
    if (leaf_pagenum == -1) return -1;

    leaf = make_page();
    file_read_page(leaf_pagenum, leaf);

    // Case: leaf has room for key and pointer.
    if (leaf->g.num_keys < LEAF_ORDER) {
        return db_insert_into_leaf(leaf_pagenum, leaf, key, pointer);
    }

    // Case: leaf must be split.
    return db_insert_into_leaf_after_splitting(leaf_pagenum, leaf, key, pointer);
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

    new_page->g.parent = 0;
    new_page->g.is_leaf = 0;
    new_page->g.num_keys = 0;
    new_page->g.next = 0;
    return new_page;
}

page_t * make_internal_page(void){
    page_t * internal = make_general_page();

    return internal;
}

page_t * make_leaf_page(void){
    page_t * leaf = make_general_page();

    leaf->g.is_leaf = 1;
    return leaf;
}

// First insertion: start a new tree.
int db_insert_new_tree(key_t key, record * pointer) {
    page_t * root = make_leaf_page();
    pagenum_t root_pagenum = file_alloc_page();

    root->g.parent = 0;
    root->g.num_keys++;
    root->g.next = 0;
    memcpy(root->g.record + 0, pointer, sizeof(record));

    header_page->h.root_pagenum = root_pagenum;

    file_write_page(root_pagenum, root);
    file_write_page(0, header_page);

    printf("db_insert_new_tree root (%lld %s)\n", root->g.record[0].key, root->g.record[0].value);

    free_page(root);
    free(pointer);
    return 0; // if success
    // return -1; if not success
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
int db_insert_into_leaf(pagenum_t leaf_pagenum, page_t * leaf, key_t key, record * pointer){
//    printf("db_insert_into_leaf\n");
    int i, insertion_point;

    insertion_point = 0;
    while (insertion_point < leaf->g.num_keys && leaf->g.record[insertion_point].key < key)
        insertion_point++;

    for (i = leaf->g.num_keys; i > insertion_point; i--) {
        memcpy(leaf->g.record + i, leaf->g.record + (i - 1), sizeof(record));
    }
    memcpy(leaf->g.record + insertion_point, pointer, sizeof(record));
    leaf->g.num_keys++;

    file_write_page(leaf_pagenum, leaf);

//    free(leaf); // can be free_leaf_page ?
//    free(pointer);

    return 0;
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
int db_insert_into_leaf_after_splitting(pagenum_t leaf_pagenum, page_t * leaf, key_t key, record * pointer) {
//    printf("db_insert_into_leaf_after_splitting\n");
    record * temp_pointers;
    page_t * new_leaf;
    pagenum_t new_leaf_pagenum;
    key_t new_key;
    int insertion_index, split, i, j;

    temp_pointers = malloc((LEAF_ORDER + 1) * sizeof(record));
    if (temp_pointers == NULL){
        perror("Temporary pointers array.");
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
    for (i = 0, j = 0; i < leaf->g.num_keys; i++, j++) {
        if (j == insertion_index) j++;
        memcpy(temp_pointers + j, leaf->g.record + i, sizeof(record));
    }

    memcpy(temp_pointers + insertion_index, pointer, sizeof(record));

    // Split page that is too big into two
    split = db_cut(LEAF_ORDER);

    leaf->g.num_keys = 0;
    for (i = 0; i < split; i++) {
        memcpy(leaf->g.record + i, temp_pointers + i, sizeof(record));
        leaf->g.num_keys++; // result : split
    }

    for (i = split, j = 0; i < LEAF_ORDER + 1; i++, j++) {
        memcpy(new_leaf->g.record + j, temp_pointers + i, sizeof(record));
        new_leaf->g.num_keys++; // result :  LEAF_ORDER + 1 - split
    }

    free(temp_pointers);
//    free(pointer);

    new_leaf->g.next = leaf->g.next;
    leaf->g.next = new_leaf_pagenum;

//    for(i = leaf->g.num_keys; i < LEAF_ORDER; i++)
//        leaf->g.record + i = NULL;
//    for (i = new_leaf->g.num_keys; i < LEAF_ORDER; i++)
//        new_leaf->g.record + i = NULL;

    new_leaf->g.parent = leaf->g.parent;
    new_key = new_leaf->g.record[0].key;

    file_write_page(leaf_pagenum, leaf);
    file_write_page(new_leaf_pagenum, new_leaf);

    return db_insert_into_parent(leaf_pagenum, leaf, new_key, new_leaf_pagenum, new_leaf);
}


/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
int db_insert_into_parent(pagenum_t left_pagenum, page_t * left, key_t key, pagenum_t right_pagenum, page_t * right) {
//    printf("db_insert_into_parent\n");
    int left_index;
    page_t * parent;
    pagenum_t parent_pagenum = left->g.parent;

    /* Case: new root. */

    if (parent_pagenum == 0)
        return db_insert_into_new_root(left_pagenum, left, key, right_pagenum, right);

    /* Case: leaf or node. (Remainder of
     * function body.)
     */

    /* Find the parent's pointer to the left
     * node.
     */
    parent = make_page();
    file_read_page(parent_pagenum, parent);

    left_index = db_get_left_index(parent, left_pagenum);

    /* Simple case: the new key fits into the node.
     */

    if (parent->g.num_keys < INTERNAL_ORDER){
//        free_page(left);
//        free_page(right);
        return db_insert_into_node(parent_pagenum, parent, left_index, key, right_pagenum);
    }

    /* Harder case:  split a node in order
     * to preserve the B+r tree properties.
     */

    return db_insert_into_node_after_splitting(parent_pagenum, parent, left_index, key, right_pagenum);
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
int db_insert_into_new_root(pagenum_t left_pagenum, page_t * left, key_t key, pagenum_t right_pagenum, page_t * right) {
//    printf("db_insert_into_new_root\n");
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

//    free_internal_page(root);
//    if (left->g.is_leaf) free_leaf_page(left);
//    else free_internal_page(left);
//    if (right->g.is_leaf) free_leaf_page(right);
//    else free_internal_page(right);

    return 0;
}

/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
int db_insert_into_node(pagenum_t n_pagenum, page_t * n,
                        int left_index, key_t key, pagenum_t right_pagenum) {
    int i;
//    printf("db_insert_into_node\n");

    // Left page is left most.
    if (left_index == -1){
        for (i = n->g.num_keys; i > 0; i--) {
            memcpy(n->g.entry + i, n->g.entry + (i - 1), sizeof(entry));
        }
    }
    else{
        for (i = n->g.num_keys; i > left_index; i--) {
            memcpy(n->g.entry + i, n->g.entry + (i - 1), sizeof(entry));
        }
    }

    n->g.entry[left_index + 1].key = key;
    n->g.entry[left_index + 1].pagenum = right_pagenum;

    n->g.num_keys++;

    file_write_page(n_pagenum, n);

//    free_internal_page(n);

    return 0;
}

/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
int db_insert_into_node_after_splitting(pagenum_t old_pagenum, page_t * old_page, int left_index,
                                        key_t key, pagenum_t right_pagenum) {
//    printf("db_insert_into_node_after_Splitting\n");
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
     * keys and pointers to the old node and
     * the other half to the new.
     */
    temp_entry = malloc((INTERNAL_ORDER + 1) * sizeof(entry));
    if (temp_entry == NULL){
        perror("Temporary entry array for splitting nodes.");
        exit(EXIT_FAILURE);
    }

    for(i = 0, j = 0; i < old_page->g.num_keys; i++, j++){
        if (j == left_index + 1) j++; // to store right page next to the left_index
        memcpy(temp_entry + j, old_page->g.entry + i, sizeof(entry));
    }

    temp_entry[left_index + 1].key = key;
    temp_entry[left_index + 1].pagenum = right_pagenum;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */
    split = db_cut(INTERNAL_ORDER); // ?

    new_page = make_page();
    new_pagenum = file_alloc_page();
    old_page->g.num_keys = 0;
    for (i = 0; i < split - 1; i++) {
        memcpy(old_page->g.entry + i, temp_entry + i, sizeof(entry));
        old_page->g.num_keys++;
    }

    k_prime = temp_entry[split - 1].key;
    new_page->g.next = temp_entry[i].pagenum;

    for (++i, j = 0; i < INTERNAL_ORDER; i++, j++) {
        memcpy(new_page->g.entry + j, temp_entry + i, sizeof(entry));
        new_page->g.num_keys++;
    }

//    free(temp_entry);
    file_write_page(old_pagenum, old_page);

    new_page->g.parent = old_page->g.parent;

    child = make_page();
    child_pagenum = new_page->g.next;
    file_read_page(child_pagenum, child);
    child->g.parent = new_pagenum;
    file_write_page(child_pagenum, child);

    for (i = 0; i < new_page->g.num_keys; i++) {
        child_pagenum = new_page->g.entry[i].pagenum;
        file_read_page(child_pagenum, child);
        child->g.parent = new_pagenum;
        file_write_page(child_pagenum, child);
    }
    file_write_page(new_pagenum, new_page);

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    return db_insert_into_parent(old_pagenum, old_page, k_prime, new_pagenum, new_page);
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int db_cut(int length) {
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}

/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to
 * the node to the left of the key to be inserted.
 */
int db_get_left_index(page_t * parent, pagenum_t left_pagenum) {
    int left_index = 0;

    if(parent->g.next == left_pagenum){
        left_index = -1;
    }
    else {
        while (left_index < parent->g.num_keys &&
               parent->g.entry[left_index].pagenum != left_pagenum)
            left_index++;
    }

    return left_index;
}
// Search

// Find the record containing input 'key'
int db_find(key_t key, char * ret_val) {
    int i;
    page_t * leaf;
    pagenum_t leaf_pagenum = db_find_leaf(key);
    if (leaf_pagenum == -1){
        return -1;
    }

    leaf = make_page();
    file_read_page(leaf_pagenum, leaf);
    for(i = 0; i < leaf->g.num_keys; i++){
        if (leaf->g.record[i].key == key) break;
    }
    if (i == leaf->g.num_keys)
        return -1;
    else{
        strcpy(ret_val, leaf->g.record[i].value);
        return 0;
    }
}

/* Traces the path from the root to a leaf, searching by key.
 * Returns the pagenum containing the given key.
 */
pagenum_t db_find_leaf(key_t key) {
    int i = 0;
    page_t * page;
    pagenum_t pagenum;

    header_page = header();
    if (header_page->h.root_pagenum == 0){
        printf("DB_find_leaf : Empty tree.\n");
        return -1;
    }

    page = make_page();
    pagenum = header_page->h.root_pagenum;
    file_read_page(pagenum, page);
    while(!page->g.is_leaf){
        i = 0;
        while (i < page->g.num_keys) {
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


// Find the matching record and delete it if found
int db_delete(key_t key){

}

pagenum_t db_queue[MAX];
int front = -1;
int rear = -1;
int q_size = 0;

int IsEmpty() {
    if (front == rear) return 1;
    else return 0;
}

int IsFull(){
    if ((rear + 1) % MAX == front) return 1;
    else return 0;
}

void db_enqueue(pagenum_t pagenum) {
    if (!IsFull()) {
        rear = (rear + 1) % MAX;
        db_queue[rear] = pagenum;
        q_size++;
    }
}

pagenum_t db_dequeue(){
    if (IsEmpty()) return 0;
    else {
        q_size--;
        front = (front + 1) % MAX;
        return db_queue[front];
    }
}


// Print

void db_print_tree() {
    int i = 0;
    page_t * page = make_page();
    header_page = header();
    printf("----------\n");
    if (header_page->h.root_pagenum == 0) {
        printf("Tree is empty\n");
        return;
    }
    db_enqueue(header_page->h.root_pagenum);
    printf("\n");

    while (!IsEmpty()) {
        int temp_size = q_size;

        while (temp_size) {
            pagenum_t pagenum = db_dequeue();
            printf("pagenum %lld ",pagenum);
            file_read_page(pagenum, page);

            if (page->g.is_leaf) {
                printf("leaf : ");
                for (i = 0; i < page->g.num_keys; i++) {
                    printf("(%lld, %s) ", page->g.record[i].key, page->g.record[i].value);
                }
                printf(" | ");
            }

            else {
                printf("internal : ");
                if (page->g.num_keys > 0){
                    printf("[%llu] ", page->g.next);
                    db_enqueue(page->g.next);
                }
                for (i = 0; i < page->g.num_keys; i++) {
                    printf("%lld [%llu] ", page->g.entry[i].pagenum, page->g.entry[i].key);
                    db_enqueue(page->g.entry[i].pagenum);
                }
//                if (i == INTERNAL_ORDER){
//                    printf("[%llu] ", page->g.next);
//                    db_enqueue(page->g. next);
//                }
//                else {
//                    printf("[%llu] ", page->g.entry[i].pagenum);
//                    db_enqueue(page->g.entry[i].pagenum);
//                }
                printf(" | ");
            }

            temp_size--;
        }
        printf("\n");
    }

    db_enqueue(header_page->h.root_pagenum);

    printf("\n");

    while (!IsEmpty()) {
        int temp_size = q_size;

        while (temp_size) {
            pagenum_t pagenum = db_dequeue();

            file_read_page(pagenum, page);

            if (page->g.is_leaf) {
                printf("leaf pagenum : %llu, parent : %llu, is_leaf : %d, num keys : %d, right sibling : %llu",
                       pagenum, page->g.parent, page->g.is_leaf, page->g.num_keys, page->g.next);
                printf(" | ");
            }

            else {
                printf("internal pagenum : %llu, parent : %llu, is_leaf : %d, num keys : %d, one more : %llu",
                       pagenum, page->g.parent, page->g.is_leaf, page->g.num_keys, page->g.next);

                db_enqueue(page->g.next);
                for (i = 0; i < page->g.num_keys; i++) {
                    db_enqueue(page->g.entry[i].pagenum);
                }
//                if (i == internal_order - 1){
//                    enqueue(page->p.one_more_pagenum);
//                }
//                else {
//                    enqueue(page->p.i_records[i].pagenum);
//                }

                printf(" | ");
            }

            temp_size--;
        }
        printf("\n");
    }

    printf("\n");
    free_page(page);
}