#include "bpt.h"
#include "file.h"
#include "dsm.h"
#include "db.h"

// MAIN

int main( int argc, char ** argv ){
    char input_file[1000];
    char data_file[1000];
    FILE * fp;
    char instruction;

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
                insert(key, value, false);
            }
            else if (instruction == 'd'){
                fscanf(fp, "%lld\n", &key);
                delete(key, false);
            }
            else if (instruction == 'f'){
                fscanf(fp, "%lld\n", &key);
                find(key, false);
            }
        }
        fclose(fp);
        print_tree();
    }

    printf("> ");
    while (scanf("%c", &instruction) != EOF){
        switch (instruction){
        case 'o':
            scanf("%s", data_file);
            table_id = open_table(data_file);
            break;
        case 'd':
            scanf("%lld", &key);
            delete(key, true);
            break;
        case 'i':
            scanf("%lld %s", &key, value);
            insert(key, value, true);
            break;
        case 'f':
            scanf("%lld", &key);
            find(key, true);
            break;
        case 'p':
            print_tree();
            break;
        case 'x':
            // make it empty
            print_tree();
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
