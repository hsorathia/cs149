#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define BUCKETS 256
#define NUMCPUS 4
int size = 0;
struct node_t
{
    char *key;
    int count;
    struct node_t *next;
};

struct queue_t
{
    struct node_t *head;
    struct node_t *tail;
    pthread_mutex_t head_lock, tail_lock;
};

struct hash_t
{
    struct queue_t *queues;
};

struct thread_info
{
    pthread_t thread_id;
    int thread_num;
    char *argv_string;
};

struct counter_t
{
    pthread_mutex_t c_lock;
    pthread_t thread_id;
    struct hash_t *hash;
};

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t test = PTHREAD_COND_INITIALIZER;
// pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t test = PTHREAD_COND_INITIALIZER;
struct queue_t *queue;
struct counter_t *counter;
int doneFlag = 0;
void *process_counters(void *arg);
/*
  This function is called whenever a new thread is created. It tries to open a
  file and inserts the words (as long as they're unique) into our table.
*/
void *process_file(void *arg);
/* 
  This function is used to initialize the head nodes of all the queues.
*/
void Queue_Init(struct queue_t *q);
/* 
  This function is used to insert a new key into the front of the queue
*/
void Queue_Enqueue(struct queue_t *q, char *key);

int Queue_Dequeue(struct queue_t *q, char *key);
/* 
  This function is used to look up a value in the queue. It returns -1 if the
  value is not found, and 0 if the value is found.
*/
// int Queue_Lookup(struct queue_t *q, char* key);
int Queue_Lookup(struct queue_t *q, char *key);
/* 
  This function is used to find the size of a given queue.
*/
int Queue_Size(struct queue_t *q);
/* 

pthread_mutex_unlock(&p->tail_lock);
  nodes in the queue.
*/
void Queue_Free(struct queue_t *q);
/*
  This function is called whenever a new thread is created. It tries to open a
  file and inserts the words (as long as they're unique) into our table.
*/
void *process_file(void *arg);

/* 
  This function initializes a hash table.
*/
void Hash_Init(struct hash_t *H);
/* 
  This function inserts a new key into the hash table.
*/
void Hash_Insert(struct hash_t *H, char *key);
/* 
  This function looks up a value in the hashtable and returns if it was found or
  not.
*/
void Hash_Lookup(struct hash_t *H, char *key);
/* 
  This function is used to find the total size of a hashtable (when we count)
  at the end.
*/
int Hash_Size(struct hash_t *H);
/* 
  This function is used to iterate through the hashtable and free all the lists.
*/
void Hash_Free(struct hash_t *H);
void Hash_Print(struct hash_t *H);

void update_counter(struct counter_t *c, char *key);
void counter_queue(struct queue_t *q);
int counter_size(struct counter_t *c);
int find_max(struct queue_t *q);
void push_back(struct queue_t *q, char *key, int count);
void Queue_Printer(struct queue_t *q);

int main(int argc, char **argv)
{
    // we begin by allocating space for the queue that is going to be used by
    // all the file proccessing threads
    queue = malloc(sizeof(struct queue_t));
    Queue_Init(queue);
    int count = 0;

    if (argc >= 2)
    {
        int i, s, amountOfThreads = argc - 1;
        struct thread_info *thread;
        void *res;

        // make space for counter threads and the file processing threads
        counter = calloc(NUMCPUS, sizeof(struct counter_t));
        if (counter == NULL)
            perror("calloc");

        thread = calloc(amountOfThreads, sizeof(struct thread_info));
        if (thread == NULL)
            perror("calloc");

        // generate the counter threads
        for (i = 0; i < NUMCPUS; i++)
        {
            // we want to make a hashtable for each counter (where the final
            // words & their count will be stored)
            struct hash_t *myHash = malloc(sizeof(struct hash_t));
            counter[i].thread_id = i;
            counter[i].hash = myHash;
            s = pthread_create(&counter[i].thread_id, NULL, &process_counters,
                               &counter[i]);
            if (s != 0)
            {
                printf("some error occurred\n");
                exit(2);
            }
        }

        // generate the file processing threads
        for (i = 0; i < amountOfThreads; i++)
        {
            thread[i].thread_num = i + 1;
            thread[i].argv_string = argv[i + 1];
            s = pthread_create(&thread[i].thread_id, NULL, &process_file,
                               &thread[i]);
            if (s != 0)
            {
                printf("some error occurred");
                exit(2);
            }
            // we want to wait until each thread is done processing
            pthread_cond_wait(&test, &lock);
        }

        for (i = 0; i < amountOfThreads; i++)
        {
            s = pthread_join(thread[i].thread_id, &res);
            if (s != 0)
            {
                printf("some error occurred");
                exit(2);
            }
        }
        // pthread_mutex_lock(&lock);
        // if (doneFlag == 0) {
        //     doneFlag = 1;
        //     counter_queue(queue);
        // }
        // pthread_cond_broadcast(&test);
        // pthread_mutex_unlock(&lock);
        // pthread_cond_wait(&test,)
        int j = 0;
        for (i = 0; i < NUMCPUS; i++) {
            int max = find_max(queue);
            if (max >= j) {
                j = max;
            } 
        }
        // printf("%d\n", j);
        counter_queue(queue);
        for (i = 0; i < NUMCPUS; i++)
        {
            // count += counter_size(&counter[i]);
            // counter_queue(queue);
            // Queue_Printer(counter[i].hash->queues);
            // find_max(counter[i].hash->queues);


            // pthread_cond_wait(&test, &lock);
            s = pthread_join(counter[i].thread_id, &res);
            if (s != 0)
            {
                printf("some error occurred");
                exit(2);
            }
        }
        free(res);
        free(thread);
        // count = Hash_Size(H1);
        // Hash_Free(H1);
        // free(H1);
        printf("count = %d\n", count);
    }
    // printf("%d\n", count);
    return 0;
}

void *process_counters(void *arg)
{
    // here we just initialize counter threads because we don't want to do 
    // anything with it yet
    struct counter_t *curr = arg;
    Hash_Init(curr->hash);
    pthread_mutex_init(&counter->c_lock, NULL);
    return NULL;
}

void *process_file(void *arg)
{
    // grab the information about the current thread and try to open the file
    // associated with the thread
    pthread_mutex_lock(&lock);
    struct thread_info *curr = arg;
    FILE *fh = fopen(curr->argv_string, "r");
    if (fh == NULL)
    {
        perror(curr->argv_string);
    }
    else
    {
        // file opening went smoothly so now we lock this thread because we want
        // it to scan the file it was given and put it in the queue

        char *ptr;
        while (fscanf(fh, "%ms", &ptr) != EOF)
        {
            Queue_Enqueue(queue, ptr);
        }
        

        printf("%d\n", find_max(queue));
        // find_max(queue);
        // printf("file words count: %d\n", count);
        // printf("count right here: %d\n", Queue_Size(queue));
        // printf("count right: %d\n", size);
        // counter_queue(queue);
        // Queue_Printer(queue);
        fclose(fh);
    }

    pthread_cond_signal(&test);
    pthread_mutex_unlock(&lock);
    return NULL;
}

void Queue_Init(struct queue_t *q)
{
    // &L = calloc(BUCKETS, sizeof(struct queue_t));
    struct node_t *new = malloc(sizeof(struct node_t));
    new->count = 0;
    q->head = new;
    q->tail = new;
    // q->head = q->tail = new;
    pthread_mutex_init(&q->head_lock, NULL);
    pthread_mutex_init(&q->tail_lock, NULL);
}

/* 
  before inserting into the queue, we want to ensure that the value does not
  exist. if it exists, that means that the count is what should be incremented.
*/
void Queue_Enqueue(struct queue_t *q, char *key)
{   
    size += 1;
    // create a new thread to store the value
    struct node_t *new = malloc(sizeof(struct node_t));
    if (new == NULL)
    {
        perror("malloc");
        return;
    }
    // lock the head/tail as we don't want any other thread to access the queue
    // while we insert
    pthread_mutex_lock(&q->head_lock);
    pthread_mutex_lock(&q->tail_lock);
    // check to see if the string exists in the qeueu
    if (Queue_Lookup(q, key) != 0)
    {
        // this is the case where it doesn't exist in the queue already
        char *value = malloc(strlen(key) + 1);
        strcpy(value, key);
        new->key = value;
        // set the count to 1 because this is the first instance of the word
        new->count = 1;
        q->tail->next = new;
        q->tail = new;
    }
    else
    {
        // it existed in the queue so we don't need the node anymore
        free(new);
    }
    // unlock and go on our merry way
    pthread_mutex_unlock(&q->tail_lock);
    pthread_mutex_unlock(&q->head_lock);
}

int Queue_Dequeue(struct queue_t *q, char *key)
{
    pthread_mutex_lock(&q->head_lock);
    struct node_t *new = q->head;
    struct node_t *new_head = new->next;
    if (new_head == NULL)
    {
        pthread_mutex_unlock(&q->head_lock);
        return -1;
    }
    key = new_head->key;
    q->head = new_head;
    free(new);
    pthread_mutex_unlock(&q->head_lock);
    return 0;
}

// int Queue_Lookup(struct queue_t *q, char* key) {
int Queue_Lookup(struct queue_t *q, char *key)
{
    int retval = -1;
    struct node_t *curr = q->head;
    while (curr)
    {
        if (curr->key != NULL)
        {
            if (strncmp(curr->key, key, strlen(curr->key)) == 0)
            {
                curr->count += 1;
                // retval = 0;
                return 0;

            }
        }
        curr = curr->next;
        if (curr == q->tail)
        {
            if (curr->key != NULL)
            {
                if (strncmp(curr->key, key, strlen(curr->key)) == 0)
                {
                    curr->count += 1;
                    // retval = 0;
                    return 0;


                }
            }
        }
    }
    return retval;
}

int Queue_Size(struct queue_t *q)
{
    struct node_t *curr = q->head;
    int count = 0;
    while (curr)
    {
        if (curr->key != NULL)
        {
            count += 1;
        }
        curr = curr->next;
        if (curr == q->tail) {
            count += 1;
        }
    }
    free(curr);
    return count;
}

void Queue_Free(struct queue_t *q)
{
    struct node_t *first, *second;
    first = q->head;
    second = q->head;
    while (first)
    {
        first = first->next;
        free(second->key);
        free(second);
        second = first;
    }
}

void Hash_Init(struct hash_t *H)
{
    struct queue_t *q = malloc(sizeof(struct queue_t));
    H->queues = q;
    Queue_Init(H->queues);
}

void Hash_Insert(struct hash_t *H, char *key)
{
    Queue_Enqueue(H->queues, key);
}

void Hash_Lookup(struct hash_t *H, char *key)
{
    // int hash_location = hash % BUCKETS;
    Queue_Lookup(H->queues, key);
}

int Hash_Size(struct hash_t *H)
{
    int count = 0;
    count += Queue_Size(H->queues);
    return count;
}

void Hash_Free(struct hash_t *H)
{
    // printf("here\n");
    int i;
    for (i = 0; i < BUCKETS; i++)
    {
        Queue_Free(H->queues);
    }
}

void update_counter(struct counter_t *c, char *key)
{
    pthread_mutex_lock(&c->c_lock);

    pthread_mutex_unlock(&c->c_lock);
}

void counter_queue(struct queue_t *q)
{
    pthread_mutex_lock(&q->head_lock);
    pthread_mutex_lock(&q->tail_lock);
    struct node_t *curr = q->head;
    while (curr)
    {
        if (curr->key != NULL)
        {
            // printf("%s\n", curr->key);
            char ch = curr->key[0];
            char mask = 0x03;
            ch = mask & ch;
            int store = ch % 4; // 4 beacuse we have four counters
            Hash_Insert(counter[store].hash, curr->key);
        }
        curr = curr->next;
        if (curr == q->tail) {
            if (curr->key != NULL)
            {
                char ch = curr->key[0];
                char mask = 0x03;
                ch = mask & ch;
                int store = ch % 4; // 4 beacuse we have four counters
                // printf("%s\n", curr->key);
                Hash_Insert(counter[store].hash, curr->key);
            }
        }
    }

    // while(q)
    pthread_mutex_unlock(&q->tail_lock);
    pthread_mutex_unlock(&q->head_lock);
}

int counter_size(struct counter_t *c)
{
    int count = 0;
    // pthread_mutex_lock(&lock);
    count += Hash_Size(c->hash);
    // pthread_cond_broadcast(&test);
    // pthread_mutex_unlock(&lock);
    return count;
}

int find_max(struct queue_t *q)
{
    // printf("at the end now\n");
    struct node_t *curr = q->head;
    int max = 0;
    struct queue_t *dummy_queue = malloc(sizeof(struct queue_t));
    Queue_Init(dummy_queue);
    while (curr)
    {
        if (curr->key)
        {
            // printf("%d, %d\n", curr->count, max);
            if (curr->count >= max)
            {
                max = curr->count;
                // push_back(dummy_queue, curr->key, curr->count);
            }
        }
        curr = curr->next;
        if (curr == q->tail)
        {
            if (curr->key)
            {
                if (curr->count >= max)
                {
                    max = curr->count;
                    // push_back(dummy_queue, curr->key, curr->count);
                }
            }
        }
    }
    // printf("at the end now\n");
    // Queue_Printer(dummy_queue);
    return max;
}

void push_back(struct queue_t *q, char *key, int count)
{
    pthread_mutex_lock(&q->tail_lock);
    struct node_t *node = malloc(sizeof(struct node_t));
    node->key = key;
    node->count = count;
    q->tail->next = node;
    q->tail = node;
    pthread_mutex_unlock(&q->tail_lock);
}

void Queue_Printer(struct queue_t *q)
{
    struct node_t *curr = q->head;
    while (curr)
    {
        if (curr->key)
        {
            printf("key: %s. count: %d.\n", curr->key, curr->count);
        }
        curr = curr->next;
        if (curr == q->tail)
        {
            if (curr->key)
            {
                printf("key: %s. count: %d.\n", curr->key, curr->count);
            }
        }
    }
}

void Hash_Print(struct hash_t *H)
{
    find_max(H->queues);
}