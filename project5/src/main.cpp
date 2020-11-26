#include "api.h"
#include <pthread.h>
// MAIN

void* trx_thread_func(void * arg){
    trx_begin();

}

void do_transaction(const int thread_num){
    pthread_t trx_threads[thread_num];
    for(int i = 0; i < thread_num; i++){
        pthread_create(&trx_threads[i], 0, trx_thread_func, NULL);
    }
    for(int i = 0; i < thread_num; i++){
        pthread_join(trx_threads[i], NULL);
    }
}

int main(int argc, char ** argv){
    char data_file[25];
    char instruction;
    int table_id = -1;
    int num_buf = 0;
    int trx_id = -1;

    k_t key, range;
    char value[120];

    usage();
    printf("> ");
    while (scanf("%c", &instruction) != EOF){
        switch (instruction){
        case 'b':
            scanf("%d", &num_buf);
            init_db(num_buf);
            break;
        case 'o':
            scanf("%s", data_file);
            table_id = open_table(data_file);
            db_print_table();
            break;
        case 'c':
            scanf("%d", &table_id);
            close_table(table_id);
            break;
        case 't':
            db_print_table();
            break;
        case 'T':
            do_transaction(5);
            break;
        case 'd':
            scanf("%d %ld", &table_id, &key);
            db_delete(table_id, key);
            break;
        case 'D':
            scanf("%d %ld %ld", &table_id, &key, &range);
            for(k_t i = key; i <= range; i++){
                db_delete(table_id, i);
            }
            break;
        case 'i':
            scanf("%d %ld %s", &table_id, &key, value);
            db_insert(table_id, key, value);
            break;
        case 'I':
            scanf("%d %ld %ld", &table_id, &key, &range);
            for(k_t i = key; i <= range; i++){
                strcpy(value, "test");
                db_insert(table_id, i, value);
            }
            break;
        case 'f':
            scanf("%d %ld %d", &table_id, &key, &trx_id);
            db_find(table_id, key, value, trx_id);
            break;
        case 'n':
            db_print_buffer();
            break;
        case 'p':
            scanf("%d", &table_id);
            db_print_tree(table_id);
            break;
        case 's':
            shutdown_db();
            break;
        case 'q':
            while (getchar() != (int)'\n');
            shutdown_db();
            return EXIT_SUCCESS;
        default:
            usage();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    shutdown_db();

    return EXIT_SUCCESS;
}
