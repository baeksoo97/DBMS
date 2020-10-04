#include "bpt.h"
#include "fim.h"
#include "dsm.h"

// MAIN

int main( int argc, char ** argv ) {
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
        order = atoi(argv[1]);
        if (order < MIN_ORDER || order > MAX_ORDER) {
            fprintf(stderr, "Invalid order: %d .\n\n", order);
            usage_3();
            exit(EXIT_FAILURE);
        }
    }
//
//    license_notice();
//    usage_1();
//    usage_2();

    if (argc > 2) {
        input_file = argv[2];
        fp = fopen(input_file, "r");
        if (fp == NULL) {
            perror("Failure  open input file.");
            exit(EXIT_FAILURE);
        }
        while (!feof(fp)) {
            fscanf(fp, "%lld %s\n", &key, value);
            root = insert(root, key, value);
        }
        fclose(fp);
        print_tree(root);
    }

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%lld", &key);
            root = delete(root, key);
            print_tree(root);
            break;
        case 'i':
            scanf("%lld", &key);
            scanf("%s", value);
            root = insert(root, key, value);
            print_tree(root);
            break;
        case 'f':
        case 'p':
            scanf("%lld", &key);
            find_and_print(root, key, instruction == 'p');
            break;
        case 'r':
            scanf("%lld %lld", &key, &range2);
            if (key > range2) {
                int tmp = range2;
                range2 = key;
                key = tmp;
            }
            find_and_print_range(root, key, range2, instruction == 'p');
            break;
        case 'l':
            print_leaves(root);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(root);
            break;
        case 'v':
            verbose_output = !verbose_output;
            break;
        case 'x':
            if (root)
                root = destroy_tree(root);
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
