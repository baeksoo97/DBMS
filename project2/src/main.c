#include "bpt.h"
#include "file.h"
#include "dsm.h"
#include "db.h"

// MAIN

int main( int argc, char ** argv ) {
    table_id = open_table("data_file.txt");

    char * input_file;
    FILE * fp;
    node * root;
    char instruction;
    char license_part;

    int64_t key, range2;
    char * value = (char *) malloc(sizeof(char) * 120);

    root = NULL;
    verbose_output = false;

    if (argc > 1) {
        input_file = argv[1];
        fp = fopen(input_file, "r");
        if (fp == NULL) {
            perror("Failure  open input file.");
            exit(EXIT_FAILURE);
        }
        while (!feof(fp)) {
            fscanf(fp, "%lld %s\n", &key, value);
            db_insert(key, value);
//            db_print_tree();
        }
        fclose(fp);
        db_print_tree();
    }

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
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
            if (db_find(key, value) == 0)
                printf("find key %lld -> value %s\n", key, value);
            else
                printf("find not key %lld\n", key);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 'p':
            db_print_tree();
            break;
        case 'x':
            // make it empty
            print_tree(root);
            break;
        default:
            usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");

    return EXIT_SUCCESS;
}
