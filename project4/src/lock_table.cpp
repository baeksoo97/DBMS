#include <lock_table.h>

typedef struct hash_table_t{
    table_id_t table_id;
    record_id_t record_id;
    lock_t * tail;
    lock_t * head;
}hash_table_t;

typedef struct lock_t {
    lock_t * prev;
    lock_t * next;
    hash_table_t * sentinel;
    int cond;
}lock_t;

pthread_mutex_t lock_table_latch;

int init_lock_table()
{
//    lock_table_latch = 0;
//    pthread_mutex_init(&lock_table_latch, NULL);


	return 0;
}

lock_t* lock_acquire(int table_id, int64_t key)
{
	/* ENJOY CODING !!!! */
//	pthread_mutex_lock(&lock_table_latch);

//	pthread_mutex_unlock(&lock_table_latch);

	return nullptr;
}

int lock_release(lock_t* lock_obj)
{
	/* GOOD LUCK !!! */

//    pthread_mutex_lock(&lock_table_latch);

//    pthread_mutex_unlock(&lock_table_latch);
	return 0;
}
