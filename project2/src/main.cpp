#include "api.h"
// MAIN

int main( int argc, char ** argv ){
    char input_file[1000];
    char data_file[1000];
    FILE * fp;
    char instruction;
    int table_id;

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
                fscanf(fp, "%ld %s\n", &key, value);
                printf("insert %ld %s\n", key, value);
                db_insert(key, value);
            }
            else if (instruction == 'd'){
                fscanf(fp, "%ld\n", &key);
                printf("delete %ld\n", key);
                db_delete(key);
            }
            else if (instruction == 'f'){
                fscanf(fp, "%ld\n", &key);
                printf("find %ld\n", key);
                db_find(key, value);
            }
        }
        fclose(fp);
        db_print_tree();
    }

    usage();
    printf("> ");
    while (scanf("%c", &instruction) != EOF){
        switch (instruction){
        case 'o':
            scanf("%s", data_file);
            table_id = open_table(data_file);
            break;
        case 'c':
            scanf("%s", data_file);
            close_table(data_file);
            break;
        case 'd':
            scanf("%ld", &key);
            db_delete(key);
            db_print_tree();
            break;
        case 'i':
            scanf("%ld %s", &key, value);
            db_insert(key, value);
            db_print_tree();
            break;
        case 'f':
            scanf("%ld", &key);
            db_find(key, value);
            db_print_tree();
            break;
        case 'p':
            db_print_tree();
            break;
        case 'q':
            while (getchar() != (int)'\n');
            close_table();
            return EXIT_SUCCESS;
        default:
            usage();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    close_table();

    return EXIT_SUCCESS;
}
