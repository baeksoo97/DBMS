#include "api.h"
// MAIN

int main( int argc, char ** argv ){
    char input_file[1000];
    char data_file[1000];
    FILE * fp;
    char instruction;
    int table_id = -1;

    k_t key;
    char value[120];

    if (argc > 1){
        strcpy(input_file, argv[1]);
        fp = fopen(input_file, "r");
        if (fp == NULL){
            perror("Failure  open input file.");
            exit(EXIT_FAILURE);
        }

        strcpy(data_file, argv[2]);
        table_id = open_table(data_file);
        while (!feof(fp)){
            fscanf(fp, "%c ", &instruction);
            if (instruction == 'i'){
                fscanf(fp, "%lld %s\n", &key, value);
                printf("insert %lld %s\n", key, value);
                db_insert(table_id, key, value);
            }
            else if (instruction == 'd'){
                fscanf(fp, "%lld\n", &key);
                printf("delete %lld\n", key);
                db_delete(table_id, key);
            }
            else if (instruction == 'f'){
                fscanf(fp, "%lld\n", &key);
                printf("find %lld\n", key);
                db_find(table_id, key, value);
            }
        }
        fclose(fp);
        db_print_tree(table_id);
        close_table(table_id);
    }

    usage();
    printf("> ");
    while (scanf("%c", &instruction) != EOF){
        switch (instruction){
        case 'o':
            scanf("%s", data_file);
            table_id = open_table(data_file);
            printf("table_id %d\n", table_id);
            break;
        case 'c':
            scanf("%d", &table_id);
            close_table(table_id);
            break;
        case 'd':
            scanf("%d %lld", &table_id, &key);
            db_delete(table_id, key);
            db_print_tree(table_id);
            break;
        case 'i':
            scanf("%d %lld %s", &table_id, &key, value);
            db_insert(table_id, key, value);
            db_print_tree(table_id);
            break;
        case 'f':
            scanf("%d %lld", &table_id, &key);
            db_find(table_id, key, value);
            db_print_tree(table_id);
            break;
        case 'p':
            scanf("%d", &table_id);
            db_print_tree(table_id);
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
        printf(" > ");
    }
    printf("\n");
    shutdown_db();

    return EXIT_SUCCESS;
}
