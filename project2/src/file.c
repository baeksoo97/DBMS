#include "file.h"

int file_id = -1;
int table_id = -1;
page_t * header_page = NULL;

/* Creates a new page, which can be adapted
 * to serve as a head, a free, a leaf or an internal page.
 */
page_t * make_page(){
    page_t * page = malloc(sizeof(page_t));
    if (page == NULL){
        perror("Page creation.");
        exit(EXIT_FAILURE);
    }
    return page;
}

void free_page(page_t * page){
    free(page);
}

void file_init_header(int isExist){
    header_page = make_page();

    if (!isExist){
        header_page->h.free_pagenum = 0;
        header_page->h.root_pagenum = 0;
        header_page->h.num_pages = 1;
        file_write_page(0, header_page);
    }
}

page_t * header(){
    file_read_page(0, header_page);
    return header_page;
}

void file_init_root(const page_t * root){
    pagenum_t root_pagenum = file_alloc_page();

    header_page->h.root_pagenum = root_pagenum;

    file_write_page(root_pagenum, root);
    file_write_page(0, header_page);
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){
    pagenum_t pagenum;
    page_t * page = make_page();

    header_page = header();

    // Allocate an on-disk page more
    if (header_page->h.free_pagenum == 0){
        off_t file_size;
        pagenum_t start_free_pagenum, curr_free_pagenum, added_free_pagenum;

        file_size = lseek(file_id, 0, SEEK_END);

        // Guarantee the size of file to be the unit of PAGE_SIZE(4096)
        if (file_size % PAGE_SIZE != 0){
            file_size = (file_size / PAGE_SIZE + 1) * PAGE_SIZE;
            if (ftruncate(file_id, file_size)){
                printf("Error : it cannot truncate to size %lld\n", file_size);
                exit(EXIT_FAILURE);
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

    printf("Alloc Page %lld (h.free_pagenum %lld)\n", pagenum, header()->h.free_pagenum);

    free_page(page);

    return pagenum;
}

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
    page_t * page = make_page();
    header_page = header();

    page->f.next_free_pagenum = header_page->h.free_pagenum;
    header_page->h.free_pagenum = pagenum;

    printf("file_free_page header->free_page %lld, free_pagenum %lld, free_pagenum->next %lld\n",
           header_page->h.free_pagenum, pagenum, page->f.next_free_pagenum);

    file_write_page(pagenum, page);
    file_write_page(0, header_page);

    free_page(page);
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
    lseek(file_id, pagenum * PAGE_SIZE, SEEK_SET);
    ssize_t read_size = read(file_id, dest, sizeof(*dest));

    if (read_size != PAGE_SIZE){
        printf("ERROR READ_PAGE : pagenum %lld, read size %zd\n", pagenum, read_size);
        exit(EXIT_FAILURE);
    }
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
    lseek(file_id, pagenum * PAGE_SIZE, SEEK_SET);
    ssize_t write_size = write(file_id, src, sizeof(*src));

    if (write_size != PAGE_SIZE){
        printf("ERROR WRITE_PAGE : pagenum %lld, write size %zd\n", pagenum, write_size);
        exit(EXIT_FAILURE);
    }
}
