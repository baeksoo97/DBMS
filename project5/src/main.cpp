#include "api.h"
#include <pthread.h>
// MAIN
#define TEST_NUM 10
#define KEY_NUM 20

char update_val[120] = "t";
string s_update = "";
unordered_map<int , vector<pair<int, k_t> > > trx_result;
unordered_map<int, vector<pair<int, k_t> > >::iterator trx_it;

// void* trx_thread_func(void * args){
//     char ret_val[120];
//     int table_id, trx_id, result = 0;
//     k_t key;

//     trx_id = trx_begin();
//     printf("***********TRX_BEGIN - trx_id : %d, thread_id : %u\n", trx_id, pthread_self());

//     for(int i = 0; i < TEST_NUM; i++){
//         table_id = i % TABLE_NUM + 1;
//         key = (i + 1) * 2;
//         result = db_find(table_id, key, ret_val, trx_id);

//         if (result == 0){
//             printf("TEST FIND success - trx_id %d, thread_id %lu, table_id %d, key %d, ret_val %s\n", trx_id, pthread_self(), table_id, key, ret_val);
//         }
//         else{
//             printf("TEST FIND abort - trx_id %d, thread_id %lu, table_id %d, key %d\n", trx_id, pthread_self(), table_id, key);
//             break;
//         }
//     }

// //    if (result == 0){
// //        for(int i = 0; i < TEST_NUM; i++){
// //            table_id = i % TABLE_NUM + 1;
// //            key = (i + 1) * 2;
// //            s_update = "t" + to_string(key);
// //            strcpy(update_val, s_update.c_str());
// //            result = db_update(table_id, key, update_val, trx_id);
// //            if (result == 0){
// //                printf("TEST UPDATE success - trx_id %d, thread_id %u, table_id %d, key %d, ret_val %s\n", trx_id, pthread_self(), table_id, key, update_val);
// //
// //            }
// //            else{
// //                printf("TEST UPDATE abort - trx_id %d, thread_id %u, table_id %d, key %d\n", trx_id, pthread_self(), table_id, key);
// //                break;
// //            }
// //        }
// //    }

//     if (result == 0){
//         trx_commit(trx_id);
//         printf("\n*** TEST COMMIT success - trx_id %d, thread_id %lu\n", trx_id, pthread_self());
//     }
//     else{
//         printf("\n*** TEST COMMIT abort - trx_id %d, thread_id %lu\n", trx_id, pthread_self());
//     }

//     return NULL;

// }

void do_transaction(const int thread_num){
    // trx_result.clear();
    // char t1[10] = "t1.db", t2[10] = "t2.db", t3[10] = "t3.db";
    // init_db(10);
    // open_table(t1);
    // open_table(t2);
    // open_table(t3);

    // pthread_t trx_threads[thread_num];
    // for(int i = 0; i < thread_num; i++){
    //     pthread_create(&trx_threads[i], 0, trx_thread_func, NULL);
    // }
    // for(int i = 0; i < thread_num; i++){
    //     pthread_join(trx_threads[i], NULL);
    // }

    // for (trx_it = trx_result.begin(); trx_it != trx_result.end(); trx_it++) {
    //     printf("Transaction %d - ", trx_it->first);
    //     for (int i = 0; i < trx_it->second.size(); i++)
    //         printf("(%d, %lld) ", trx_it->second[i].first, trx_it->second[i].second);
    //     printf("\n");
    // }
    printf("transaction done\n");
}

int main(int argc, char ** argv){
    char data_file[25];
    char instruction;
    int table_id = -1;
    int num_buf = 0;
    int trx_id = -1;
    int thread_num = 0;
    string s_value;

    k_t key, range;
    char value[120];

    print_usage();
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
            scanf("%d", &thread_num);
            do_transaction(thread_num);
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
                s_value = "test";
                s_value += to_string(i);
                strcpy(value, s_value.c_str());

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
            print_usage();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    shutdown_db();

    return EXIT_SUCCESS;
}
