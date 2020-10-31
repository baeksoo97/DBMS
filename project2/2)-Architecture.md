# Architecture

The system architecture for disk-based b+tree is organized by 4 layers like the above : User layer, API layer, Index layer, and Disk layer.

![bpt_architecure](uploads/78a437a066904de1db023fa472629fc5/bpt_architecure.png)

## Disk Layer

Disk layer is a layer which access directly to data file(disk). It conducts the disk I/O task for opening or closing the data file, reading or writing a page in the data file, and allocating new pages in the data file.

### `file_open()`

- This is a function for opening the file specified by `pathname` using the `open()` system call. On succeed, a file descriptor which represents the unique table id **(in this project, there is only one table in each file so that the file descriptor is used as the table id, but it would be changed in future assignment)** is returned; or not, -1 is returned.
    - If the specified file does not exist, it creates a new file named by pathname. And it initializes a header page with the default; `free_pagenum` is 0, `root_pagenum` is 0, `num_pages` is 1 including header.
    - If the specified file already exists, it initializes a header page by retrieving the first page in the file.
- The return value of `open()`, file descriptor, should be stored in the global variable, `file_id`. This nonnegative integer value is used in subsequent system calls (`read()`, `write()`, `lseek()`) to refer to the data file.

    ```cpp
    int file_open(const char * pathname){
        int fd;

        if (!access(pathname, F_OK)){
            // file exists
            fd = open(pathname, O_RDWR, 0644); // open, read, write
            if (fd == -1){
                perror("ERROR FILE_OPEN : cannot open file\n");
                return -1;
            }
            printf("open already existing file : %d\n", fd);
            FILE_ID = fd;
            file_init_header(1);
        }
        else{
            // file doesn't exist
            fd = open(pathname, O_CREAT | O_RDWR, 0644); // create new, read, write
            if (fd == -1){
                perror("ERROR FILE_OPEN : cannot create file\n");
                return -1;
            }
            printf("create new file : %d\n", fd);
            FILE_ID = fd;
            file_init_header(0);
        }

        return fd;
    }
    ```

### `file_alloc_page()`

- This is a function for allocating an on-disk page from the free page list.
    - If there are no more free pages to be allocated in disk, new free pages should be created. I implement 10000(`PAGE_NUM_FOR_RESERVE`) free pages to be create once there are no free pages. Also, I update the `next_free_pagenum` of each free page(that of the last free page should be 0), the `free_pagenum` of header, the `num_pages` of header.
    - It returns the `free_pagenum` of header which is the first free page in the free page list. According this, the `free_pagenum` of header should be updated to the `next_free_pagenum` of the original first free page.

    ```cpp
    #define PAGE_NUM_FOR_RESERVE 10000
    pagenum_t file_alloc_page(){
        pagenum_t pagenum;
        page_t * page = make_page();

        header_page = header();

        // Allocate an on-disk page more
        if (header_page->h.free_pagenum == 0){
            off_t file_size;
            pagenum_t start_free_pagenum, curr_free_pagenum, added_free_pagenum;

            file_size = lseek(FILE_ID, 0, SEEK_END);
            if (file_size == -1){
                perror("ERROR FILE_ALLOC_PAGE");
            }

            // Guarantee the size of file to be the unit of PAGE_SIZE(4096)
            if (file_size % PAGE_SIZE != 0){
                file_size = (file_size / PAGE_SIZE + 1) * PAGE_SIZE;
                if (ftruncate(FILE_ID, file_size)){
                    perror("ERROR FILE_ALLOC_PAGE");
                }
            }

            start_free_pagenum = file_size / PAGE_SIZE;
            curr_free_pagenum = start_free_pagenum;
            for(int i = 0; i < PAGE_NUM_FOR_RESERVE; i++){
                if (i < PAGE_NUM_FOR_RESERVE - 1)
                    page->f.next_free_pagenum = curr_free_pagenum + 1;
                else
                    page->f.next_free_pagenum = 0; // the end of free page list

                file_write_page(curr_free_pagenum, page);
                curr_free_pagenum++;
            }

            added_free_pagenum = curr_free_pagenum - start_free_pagenum;

            header_page->h.free_pagenum = start_free_pagenum;
            header_page->h.num_pages += added_free_pagenum;
        }

        pagenum = header_page->h.free_pagenum;

        // Get free page from on-disk
        file_read_page(pagenum, page);

        // Update header page to on-disk
        header_page->h.free_pagenum = page->f.next_free_pagenum;
        file_write_page(0, header_page);

        // printf("Alloc Page %lld (h.free_pagenum %lld)\n", pagenum, header()->h.free_pagenum);

        free_page(page);

        return pagenum;
    }
    ```

### `file_free_page()`

- This is a function for making an on-disk page to be a free page.
    - First, it creates a page that will be substituted for a free page which number is `pagenum`.
    - This new free page should be placed between the header and it's the first pointing free page.
    - After updating the `next_free_pagenum` of this free page and header page, it should be also updated in the data file by calling `file_write_page()`.

    ```cpp
    void file_free_page(pagenum_t pagenum){
        page_t * page = make_page();
        header_page = header();

        page->f.next_free_pagenum = header_page->h.free_pagenum;
        header_page->h.free_pagenum = pagenum;

        file_write_page(pagenum, page);
        file_write_page(0, header_page);

        free_page(page);
    }
    ```

### `file_read_page()`

- This is a function for reading an on-disk page into the in-memory page structure.
    - It repositions the file offset of the data file to the position of page using system call `lseek()`. The position of page is calculated as `its page number * PAGE_SIZE`.
    - The system call `read()` attempts to read up to the size of page from the data file into the buffer starting at dest. If the return values of `read()` is not same as the constant, `PAGE_SIZE`, it is on error so that it terminates the process by calling `exit()`.

    ```cpp
    void file_read_page(pagenum_t pagenum, page_t* dest){
        lseek(FILE_ID, pagenum * PAGE_SIZE, SEEK_SET);
        ssize_t read_size = read(FILE_ID, dest, PAGE_SIZE);

        if (read_size == -1)
            perror("ERROR READ_PAGE");
        else if (read_size != PAGE_SIZE)
            printf("ERROR READ_PAGE : pagenum %lu, read size %zd\n", pagenum, read_size);
    }
    ```

### `file_write_page()`

- This is a function for writing an in-memory page to the on-disk page.
    - It repositions the file offset of the data file to the position of page using system call `lseek()`. The position of page is calculated as `its page number * PAGE_SIZE`.
    - The system call `write()` attempts to write up to the size of page from the buffer starting at src to the data file. If the return value of `write()` is not the same as the constant, `PAGE_SIZE`, it is on error so that it terminates the process by calling `exit()`.

    ```cpp
    void file_write_page(pagenum_t pagenum, const page_t* src){
        lseek(FILE_ID, pagenum * PAGE_SIZE, SEEK_SET);
        ssize_t write_size = write(FILE_ID, src, PAGE_SIZE);
        fsync(FILE_ID);

        if (write_size == -1)
            perror("ERROR WRITE_PAGE");
        else if (write_size != PAGE_SIZE)
            printf("ERROR WRITE_PAGE : pagenum %lu, write size %zd\n", pagenum, write_size);
    }
    ```

## Index Layer

Index layer is a layer for b+tree. All functions in this disk version are almost the same as functions in the previous memory version except accessing and managing data. 

### Delayed Merge

In this version, structure modification occurs only when all keys in the page have been deleted regardless of the branching factor. 

1. Delayed Merge Flow
    - Case 1 : When all keys in an internal page have been deleted,
        - Case 1 - 1 : When the neighboring page can accept the one additional entry of the current page, the current page would be combined into the neighboring page.
        - Case 1 - 2 : When the neighboring page cannot accept the one additional entry of the current page, the two pages should be redistributed.
    - Case 2: When all keys in a leaf page have been deleted,
        - The current page is merged into the neighboring page regardless of whether the neighboring page is full.

    ```cpp
    if (n_page->g.num_keys >= min_keys)
            return 0;

    // all keys in the page have been deleted
    /* Coalescence. */
    if ((!n_page->g.is_leaf && neighbor->g.num_keys + n_page->g.num_keys < capacity)
        || n_page->g.is_leaf)
        return coalesce_nodes(parent_pagenum, parent, n_pagenum, n_page, neighbor_pagenum,
                              neighbor, neighbor_index, k_prime);

    /* Redistribution. */
    else
        return redistribute_nodes(parent_pagenum, parent, n_pagenum, n_page, neighbor_pagenum,
                                  neighbor, neighbor_index, k_prime_index, k_prime);
    ```

2. `redistribute_nodes()`

    When this function is called, the current page and this neighboring page would be redistributed. The half entries of the neighboring will be moved to the current page.

3. Example

    For example, please see below image.

    ![redistribution1](uploads/43385300d1e8ec56656fd9e84395763c/redistribution1.png)

    If `Delete key 4` occurs, two pages become empty.

    - The leaf page 1 : When it becomes empty, this page and it's neighboring page will be combined by `coalesce_nodes()`.
    - The internal page 3 : When it becomes empty, this page and it's neighboring page 8 will be redistributed because the neighboring page 8 is already so full to accommodate this page. Therefore, the half entries of the neighboring page 8 are moved to the internal page 3 and their parent page 9 is updated like below image.

    ![redistribute2](uploads/483fa6456408968811fb3549075e1ec4/redistribute2.png)

## API Layer

API layer is a layer for user interface. All functions can be called according to a user's command for creating data file based on B+Tree, and administering this data file. Each function in this layer calls functions in **Index layer.**

### `open_table()`

- This is a function for opening existing data file named 'pathname' or create one if it does not exist.
    - call : `index_open_table()` in Index layer
    - return
        - table_id : the unique table id
        - -1 : fail (cannot create or open file)

### `close_table()`

- This is a function for trying to close existing data file named 'pathname'.
    - call : `index_close_table()` in Index layer
    - The default value of pathname is an empty string, and `close_table()` with no argument means closing all files that have been opened.

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

## User Layer

This layer is a layer for processing the user's command input by terminal or input file. It calls some function in API layer which corresponds to user's command. 

### command in terminal

![command_in_terminal](uploads/1e74858013b110c59b26907cd1a6dc31/command_in_terminal.png)

### command with input file

- input file

    ```
    // input.txt
    i 1 1
    i 2 2
    i 3 3
    d 1
    f 1
    ```

- command

    ![command_with_input_file](uploads/25306598cd5dcebcd3383dc14b7c9337/command_with_input_file.png)