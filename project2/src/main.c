#include "api.h"
#include "file.h"
#include "dsm.h"
#include "index.h"

// MAIN

int main( int argc, char ** argv ){
    char input_file[1000];
    char data_file[1000];
    FILE * fp;
    char instruction;
    int table_id;

    key_t key;
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
                db_insert(key, value);
            }
            else if (instruction == 'd'){
                fscanf(fp, "%lld\n", &key);
                printf("delete %lld\n", key);
                db_delete(key);
            }
            else if (instruction == 'f'){
                fscanf(fp, "%lld\n", &key);
                printf("find %lld\n", key);
                db_find(key, value);
            }
        }
        fclose(fp);
        db_print_tree();
    }

    printf("> ");
    while (scanf("%c", &instruction) != EOF){
        switch (instruction){
        case 'o':
            scanf("%s", data_file);
            table_id = open_table(data_file);
            printf("table_id %d\n", table_id);
            break;
        case 'd':
            scanf("%lld", &key);
            db_delete(key);
            db_print_tree();
            break;
        case 'i':
            scanf("%lld %s", &key, value);
            db_insert(key, value);
            db_print_tree();
            break;
        case 'f':
            scanf("%lld", &key);
            db_find(key, value);
            db_print_tree();
            break;
        case 'p':
            db_print_tree();
            break;
        case 'x':
            // make it empty
            db_print_tree();
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
        default:
            usage();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");

    return EXIT_SUCCESS;
}
