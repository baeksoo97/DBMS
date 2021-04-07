# Buffer Manager

## Buffer Layout

### Buffer Header Structure

```cpp
typedef struct buffer_header_t{
    // value -1 means buffer doesn't have key pagenum
    vector <unordered_map<pagenum_t, framenum_t>> frame_map; // hash map 
    framenum_t buf_capacity; // the size of buffer
    framenum_t buf_size;     // the number of frames in buffer
    framenum_t head;         // first(mru) frame idx in LRU list
    framenum_t tail;         // last(lru) frame idx in LRU list
    bool is_open;
} buffer_header_t;
```

### Buffer Structure

```cpp
typedef struct buffer_t{
    page_t * frame;    // physical page
    int table_id;      // the unique table id of page
    pagenum_t pagenum; // page number
    bool is_dirty;     // whether the page is dirty (memory & disk version is different)
    int pin_cnt;       // the count of that the frame is being used 
    framenum_t prev;   // previous frame idx in LRU list
    framenum_t next;   // next frame idx in LRU list
} buffer_t;
```

## LRU Policy

LRU caches stand for Least Recently Used Cache which evict least recently used entry. So we have to keep track of recently used entries, entries which have not been used from long time and which have been used recently. Plus, lookup and insertion operation should be fast enough. 

### Hash Map

Cache insert and lookup operation should be fast, preferably O(1) time. Therefore, I use Hash Map for insertion and lookup.

### Doubly Linked List

 To track the recently used entries, I sed Doubly Linked List. Reason for choosing doubly linked list is O(1) deletion, update and insertion if we have the address of frame on which this operation has to perform. 

 The doubly linked list shows frames' time history by its order. The later the list is, the older the frame to use.

- The last frame in list = Least Recently Used frame = `header_page.tail`
- The first frame in list = Most Recently Used frame = `header_page.head`

![Untitled](uploads/465abab6e77928092b72530ef7c8c081/Untitled.png)

### LRU eviction

The tail of list is the Least Recently Used frame. Therefore, if eviction occurs, the tail would be the eviction frame. However, if the chosen frame is pinned, choose previous frame of it that is not pinned. 

If the eviction frame is dirty, It should be flushed from memory to disk.

### LRU order update

Whenever the most recently used frame is inserted into the list, the frame should be moved to top, the head of list. There are some cases.

- Case 1 : When new frame that have been never used is used, the frame would be just inserted at the head of list.
    - Case 1 - 1 : If buffer was empty, just put the frame at the first of list. The head & tail of list would points the frame, and the prev & next of the frame would points itself.
    - Case 1 - 2 : If buffer was not empty, put the frame at the first of list and relink the old first frame.

    ```cpp
    // When new frame that have been never used is inserted into the list
    void buffer_lru_insert(framenum_t frame_idx){
    		// Case 1 - 1 : if buffer was empty
        if (buffer_header.head == NONE && buffer_header.tail == NONE){
            buffer_header.head = frame_idx;
            buffer_header.tail = frame_idx;
            buffer[frame_idx].prev = frame_idx;
            buffer[frame_idx].next = frame_idx;
        }
    		// Case 1 - 2 : if buffer was not empty
        else{
            buffer[frame_idx].prev = buffer_header.tail;
            buffer[frame_idx].next = buffer_header.head;
            buffer[buffer_header.head].prev = frame_idx;
            buffer[buffer_header.tail].next = frame_idx;
            buffer_header.head = frame_idx;
        }
    }
    ```

- Case 2 : When the used frame is used again, the order of frame would be changed in the list.
    - Case 2 - 1 : If head already points the frame, just skip the reordering.
    - Case 2 - 2: If tail already points the frame, point tail to the previous frame of the frame and update linking.
    - Case 2 - 3 : Just update linking like below code.

    ```cpp
    // update lru list
    void buffer_lru_update(framenum_t frame_idx){
        // frame_idx is already mru
        if (!buffer_header.is_open)
            return;

        if (buffer_header.head == frame_idx)
            return;

        if (buffer_header.tail == frame_idx){
            buffer_header.tail = buffer[frame_idx].prev;
        }

        // get frame_idx out from list
        buffer[buffer[frame_idx].prev].next = buffer[frame_idx].next;
        buffer[buffer[frame_idx].next].prev = buffer[frame_idx].prev;
        buffer[frame_idx].next = buffer_header.head;
        buffer[frame_idx].prev = buffer_header.tail;
        buffer[buffer_header.head].prev = frame_idx;
        buffer[buffer_header.tail].next = frame_idx;
        buffer_header.head = frame_idx;
    }
    ```
    
    
# Architecture

![architecture](uploads/a8c2831d28db1a179d321977281673a1/architecture.png)

The system architecture for disk-based B+Tree with buffer manager is organized by 5 layers like the above : User layer, API layer, Index layer, Buffer layer, and Disk layer.

Each layer calls its own function or function of adjacent layers and uses its own resources or resources from adjacent layers to keep order between layers. For example, the buffer is only used in Buffer layer and the Buffer layer calls `file_read_page()` or `file_write_page()` in Disk layer.

## Disk Layer

Disk layer is a layer which access directly to data file(disk). It conducts the disk I/O task for opening or closing the data file, administering table ids, reading or writing a page in the data file, and allocating new pages in the data file.

### `file_open_table()`

- This is a function for opening a data file and allocating the unique table id for the data file.
- The file descriptor can be changed whenever the same file is opened so that it should be mapped with the same unique table id.
- The table id should be maintained once open_table() is called, which is matching file descriptor.
- In this function, file descriptor, table_id, and pathname should be stored in hash_map named `file_table_map` and `table_fd_map`.
- It also should set the status of whether this file is opened or not.
- `file_table_map` and `table_fd_map` Example

    ![hashmap](uploads/33d30a00cdd57dc3ec87fa0143fd3a31/hashmap.png)

### `file_close_table()`

- This is a function for closing already existing file which have `table_id`. It has `table_id` parameter which represents the table_id of the data file.
    - if `table_id` is nothing(0), it closes all of existing data file that is opened. And it clears the hash map for data file name and its table id.
    - if `table_id` is not 0, it closes the existing data file having `table_id` if it is opened.
    - Every time it closes the existing file, the hash map for table id and its file descriptor should be reinitialized to the default.

### `file_alloc_page()` , `file_free_page()`

- The difference from previous project is that it reads and writes pages from buffer, not from disk by calling `buffer_read_page()` and `buffer_write_page()`
- And it must unpin the completely used frame in buffer by calling `buffer_unpin_frame()`.

## Buffer Layer

Buffer layer is a layer for storing data in memory for caching. All functions can be called from Index Layer.

### `buffer_open_table()`

- This is a function for opening existing data file named 'pathname' or create one if it does not exist.
    - call : `file_open_table()` in File layer
    - return
        - table_id : the unique table id(which represents the own table in this database)
        - -1 : fail (cannot create or open file)

### `buffer_init_db()`

- This is a function for allocating the buffer pool(array) with the given number of entries. When a user calls `init_db()`, the API layer calls `index_init_db()` and then the Index layer calls `buffer_init_db()`.
- It allocates `buffer` structure array with the given number, and initializes each `buffer` structure such as `frame`, `table_id`, `pagenum`, etc to the default. And it also initializes `buffer_header` structure to the default.
- Return
    - 0 : success
    - -1 : fail (If buffer is already allocated, if the given number is 0, if it fails allocating array)

### `buffer_close_table()`

- This is a function for writing all pages of table from buffer to disk and closing existing data file.
    - if `table_id` is nothing(0), it flushes all data from buffer and destroying allocated buffer. by calling `buffer_flush_frame()` and `buffer_close_db()`.
    - if `table_id` is not 0, it writes all pages of table having `table_id` from buffer to disk by calling `buffer_flush_table()`.
- After it flushes pages from buffer to disk, it calls `file_close_table()` to close the data file.

### `buffer_close_db()`

- This is a function for destroying allocated buffer.
    - It destroys allocated buffer and reinitializes `buffer_header` variable to the default.

### `buffer_flush_table()`

- This is a function for flushing all pages of table from buffer to disk. Each frame which has same `table_id` is flushed by `buffer_flush_frame()`.

### `buffer_flush_frame()`

- This is a function for flushing a page of frame from buffer to disk.
    - If the frame is dirty, this frame would be flushed from buffer to disk.
    - The buffer should be reinitialized to the default.

    ```cpp
    void buffer_flush_frame(framenum_t frame_idx){
        int table_id;
        pagenum_t pagenum;
        frame_map_t frame_map;
        frame_map_t::iterator frame_pos;

        table_id = buffer[frame_idx].table_id;
        if (table_id == -1) return;

        if (buffer[frame_idx].is_dirty){
            pagenum = buffer[frame_idx].pagenum;
            file_write_page(table_id, pagenum, buffer[frame_idx].frame);
        }

        buffer[frame_idx].table_id = NONE;
        buffer[frame_idx].pagenum = 0;
        buffer[frame_idx].is_dirty = false;
        buffer[frame_idx].pin_cnt = 0;
    }
    ```

### `buffer_read_page()`

- This is a function for reading page from buffer or disk.
    - Case 1 : If buffer doesn't have page, it reads page from disk and insert the page into buffer.
    - Case 2 : If buffer has page, it reads from buffer and update the order of LRU list.
    - Don't forget count pin for the page.

    ```cpp
    void buffer_read_page(int table_id, pagenum_t pagenum, page_t * dest){
        if (!buffer_header.is_open)
            return file_read_page(table_id, pagenum, dest);

        framenum_t frame_idx = buffer_find_frame(table_id, pagenum);

        // if buffer doesn't have page
        if (frame_idx == -1){
            file_read_page(table_id, pagenum, dest);
            frame_idx = buffer_init_frame(table_id, pagenum, dest);
        }
        // if buffer already has page
        else{
            memcpy(dest, buffer[frame_idx].frame, sizeof(page_t));
            buffer_lru_update(frame_idx);
        }

        buffer[frame_idx].pin_cnt++;
    }
    ```
### `buffer_write_page()`

- This is a function for writing page into buffer.
    - Case 1: If buffer doesn't have page, insert the page into buffer.
    - Case 2: If buffer has page, rewrite page to the corresponding frame and update the order of LRU list
    - Don't forget count pin for the page and mark `is_dirty`

    ```cpp
    void buffer_write_page(int table_id, pagenum_t pagenum, page_t * src){
        if (!buffer_header.is_open)
            return file_write_page(table_id, pagenum, src);

        framenum_t frame_idx = buffer_find_frame(table_id, pagenum);

        // if buffer doesn't have page
        if (frame_idx == -1){
            frame_idx = buffer_init_frame(table_id, pagenum, src);
        }
        // if buffer already has page
        else{
            memcpy(buffer[frame_idx].frame, src, sizeof(page_t));
            buffer_lru_update(frame_idx);
        }
        buffer[frame_idx].is_dirty = true;
        buffer[frame_idx].pin_cnt++;
    }
    ```

## Index Layer

Index layer is a layer for b+tree. The difference from previous project is that 

### Difference from previous project

1. calling `file_read_page()` is changed to calling `buffer_read_page()`.
2. calling `file_write_page()` is changed to calling `buffer_write_page()`.

### Unpin

- Every time it calls `buffer_read_page()` or `buffer_write_page()`, it should call `buffer_unpin_frame()` after finishing a task for its page, to make the frame to be a candidate of frames that can be evicted by LRU policy.
    - For example

        In `insert_new_tree` function, it generates new root page by `buffer_alloc_page()` and the root page is written in memory. After writing new root page into memory, it means using root page is done so that its frame in buffer should be unpinned.

        Also, this function read header for updating its `root_pagenum` , so both operations of reading and writing header progress. After finishing these operations, header's frame also should be unpinned by calling `buffer_unpin_frame()`.

        ```cpp
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
            buffer_unpin_frame(table_id, root_pagenum); // using root page is done -> unpin write
            free(root);

            // update header
            header_page = buffer_read_header(table_id);
            header_page->h.root_pagenum = root_pagenum;

            buffer_write_page(table_id, 0, header_page);
            buffer_unpin_frame(table_id, 0, 2); // using header page is done -> unpin read & write
            free(header_page);

            free(pointer);

            return 0;
        }
        ```

## API Layer

API layer is a layer for user interface. All functions can be called according to a user's command for creating data file based on B+Tree, and administering this data file. Each function in this layer calls functions in **Index layer.**

### `init_db()`

- This is a function for allocating the buffer pool with the given number of entries.
    - call : `index_init_db()` in Index layer
    - return
        - 0 : success
        - -1 : fail

### `open_table()`

- This is a function for opening existing data file named 'pathname' or create one if it does not exist.
    - call : `index_open_table()` in Index layer
    - return
        - table_id : the unique table id(which represents the own table in this database)
        - -1 : fail (cannot create or open file)

### `close_table()`

- This is a function for writing all pages of this table from buffer to disk and close the table.
    - call : `index_close_table(table_id)` in Index layer
    - return
        - 0 : success
        - -1 : fail (data file is not opened)

### `shutdown_db()`

- This is a function for flushing all data from buffer and destroying allocated buffer.
    - call : `index_close_table(0)` in Index layer
    - return
        - 0 : success
        - -1 : fail (doesn't have buffer)

### `db_insert()`

- This is a function for inserting input 'key/value'(record) to data file at the right place.
    - call : `insert()` in Index layer
    - return
        - 0 : success
        - -1 : fail (data file is not opened)

### `db_find()`

- This is a function for finding the record containing input 'key' from the data file.
    - call : `find()` in Index layer
    - return
        - 0 : success, the value of the record containing input 'key' stores in `ret_val`
        - -1 : fail (data file is not opened, cannot find key)

### `db_delete()`

- This is a function for finding the matching record containing input 'key' and delete it if found.
    - call : `delete_key()`
    - return
        - 0 : success
        - -1 : fail (data file is not opened, cannot find key)



# Et cetera

## How to compile

All source files are written with C++. Therefore, `Makefile` is modified to compile C++ source files and create static library. 

### Project Hierarchy

        ```
        |___ project3
		|___ include
			|___ api.h
			|___ file.h
			|___ index.h
			|___ buffer.h
		|___ lib
			|___ libbpt.a
		|___ src
			|___ api.cpp
			|___ api.o
			|___ file.cpp
			|___ file.o
			|___ index.cpp
			|___ index.o
			|___ buffer.cpp
			|___ buffer.o
			|___ main.cpp
			|___ main.o
		|___ main
		|___ Makefile
```
