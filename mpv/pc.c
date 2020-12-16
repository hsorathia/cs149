#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define BUCKETS 256
#define NUMCPUS 4

struct node_t {
    int count;
    char *key;
    struct node_t *next;
};

struct list_t {
    struct node_t *head;
};

struct hash_t {
    struct list_t lists[BUCKETS];
};

struct queue_t {
    struct node_t *head;
    struct node_t *tail;
    pthread_mutex_t head_lock, tail_lock;
};

struct thread_info {
    pthread_t thread_id;
    char *filename;
    struct queue_t **queues;
};

struct counter_info {
    pthread_t thread_id;
    struct max_t *max;
    struct queue_t *queue;
};

struct max_t {
    struct queue_t *words;
    int maximum;
};

// this is going to be our array of queus
struct queue_t **queues;
/*
  This function is called whenever a new thread is created. It tries to open a
  file and inserts the words (as long as they're unique) into our table.
*/
void *processor_file(void *arg);
/*
  This function is responsible for doing the processing for the counter threads
*/
void *processor_counter(void *arg);
/*
  This function is responsible for handling the threads when they get created
*/
void *processor_counter(void *arg);
/*
  This function is responsible for initializing the list for the hash table
*/
void List_Init(struct list_t *L);
/* 
  This function is used to find the size of a given list.
*/
int List_Size(struct list_t *L);
/* 
  This function is used to look up a value in the list. It returns -1 if the
  value is not found, and 0 if the value is found.
*/
int List_Lookup(struct list_t *L, char *key);
/* 
  This function is used to insert a new key into the front of the list
*/
int List_Insert(struct list_t *L, char *key);
/* 
  This function iterates through a list and frees all the values (keys) and the
  nodes in the list.
*/
void List_Free(struct list_t *L);
/* 
  This function initializes a hash table.
*/
void Hash_Init(struct hash_t *H);
/* 
  This function inserts a new key into the hash table.
*/
int Hash_Insert(struct hash_t *H, char *key);
/* 
  This function looks up a value in the hashtable and returns if it was found or
  not.
*/
int Hash_Lookup(struct hash_t *H, char *key);
/* 
  This function is used to iterate through the hashtable and free all the lists.
*/
void Hash_Free(struct hash_t *H);
/* 
  This function takes in a char string and calculate a hash value to store.
*/
unsigned long hashedString(char *key);
/*
  This function initializes our queue
*/
void Queue_Init(struct queue_t *q);
/*
  Enqueues an element into the queue
*/
void Queue_Enqueue(struct queue_t *q, char *value);
/*
  Dequeues an element from the queue
*/
int Queue_Dequeue(struct queue_t *q, char **value);
/*
  Finds the the most popular words in the file
*/
struct max_t *Find_Max(struct hash_t *H);

int main(int argc, char **argv)
{
    if (argc >= 2)
    {
        int i, s, amountOfThreads = argc - 1, local_max = 0;;

        struct thread_info *threads;
        threads = calloc(amountOfThreads, sizeof(struct thread_info));

        struct counter_info *counter_threads;
        counter_threads = calloc(NUMCPUS, sizeof(struct counter_info));

        queues = calloc(NUMCPUS, sizeof(queues));

        if (queues == NULL || threads == NULL || queues == NULL)
            perror("calloc");


        // go through our queues and then initialize all of them
        for (i = 0; i < NUMCPUS; i++) {
            queues[i] = malloc(sizeof(struct queue_t));
            Queue_Init(queues[i]);
        }

        // generate all of our threads
        for (i = 0; i < amountOfThreads; i++) {
            threads[i].thread_id = i;
            threads[i].filename = argv[i + 1];
            threads[i].queues = queues;
            s = pthread_create(&threads[i].thread_id, NULL, &processor_file, 
                                    &threads[i]);
            if (s != 0) {
                printf("some error occurred");
                exit(2);
            }
        }


        for (i = 0; i < amountOfThreads; i++) {
            s = pthread_join(threads[i].thread_id, NULL);
            if (s != 0) {
                printf("some error occurred");
                exit(2);
            }
        }

        for (i = 0; i < NUMCPUS; i++) {
            counter_threads[i].thread_id = i;
            counter_threads[i].queue = queues[i];
            s = pthread_create(&counter_threads[i].thread_id, NULL, 
                                    &processor_counter, &counter_threads[i]);
            if (s != 0) {
                printf("some error occurred");
                exit(2);
            }
        }

        // up to speed now
        for (i = 0; i < NUMCPUS; i++) {
            s = pthread_join(counter_threads[i].thread_id, NULL);
            if (s != 0) {
                printf("some error occurred");
                exit(2);
            }
        }

        // we wnat to go through and then find the actual maximum value for the
        // counter threads
        for (i = 0; i < NUMCPUS; i++) {
            if (counter_threads[i].max->maximum > local_max) {
                local_max = counter_threads[i].max->maximum;
            }
        }
        
        // go through the queues that we have and then read them out while we
        // pop them
        for (i = 0; i < NUMCPUS; i++) {
            char *max_word;
            if (counter_threads[i].max->maximum == local_max) {
                while (Queue_Dequeue(counter_threads[i].max->words, &max_word) == 0) {
                    printf("%s %i\n", max_word, local_max);
                }
            }
        }
    }

    return 0;
}

void *processor_file(void *arg)
{
    struct thread_info *parameters = arg;
    FILE *fh = fopen(parameters->filename, "r");
    if (fh == NULL)
    {
        // printf("error opening file\n");
        perror(parameters->filename);
    } else {
        char *ptr;
        fscanf(fh, "%ms", &ptr);
        while (ptr != NULL) {
            int ch = (int)ptr[0] & 0x03;
            Queue_Enqueue(parameters->queues[ch], ptr);
            free(ptr);
            fscanf(fh, "%ms", &ptr);
        }

        free(ptr);
        fclose(fh);
    }

    return NULL;
}

void *processor_counter(void *arg) {
    struct counter_info *parameters = arg;

    // create a new hash table and initialize it
    struct hash_t *hash = malloc(sizeof(struct hash_t));
    if (hash == NULL) {
        perror("malloc");
        return NULL;  
    }
    Hash_Init(hash);
    char *value;
    // if we can't
    while (Queue_Dequeue(parameters->queue, &value) == 0) {
        Hash_Insert(hash, value);
    }
    parameters->max = Find_Max(hash);
    return NULL;
}

/*
  initialize the values 
*/
void List_Init(struct list_t *L) {
    L->head = NULL;
}

/* 
  before inserting into the string, check to see if the string exists in the
  list or not. if it does then we don't save the value.
*/
int List_Insert(struct list_t *L, char *key) {

    struct node_t *new = malloc(sizeof(struct node_t));
    if (new == NULL)
    {
        perror("malloc");
        return -1; // fail
    }
    if (List_Lookup(L, key) == 0) {
        return -1;
    }
    new->key = malloc(strlen(key) + 1);
    strcpy(new->key, key);
    new->count = 1;
    new->next = L->head;
    L->head = new;
    return 0;
}

int List_Size(struct list_t *L) {
    struct node_t *curr = L->head;
    int listMax = 0;
    while (curr) {
        if (curr->count > listMax) {
            listMax = curr->count;
        }
        curr = curr->next;
    }
    return listMax;
}

void List_Free(struct list_t *L) {
    struct node_t *curr = L->head;
    struct node_t *prev = L->head;
    while (curr) {
        curr = curr->next;
        free(prev->key);
        free(prev);
        prev = curr;
    }
}

int List_Lookup(struct list_t *L, char *key) {
    struct node_t *curr = L->head;
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            curr->count += 1;
            return 0;
        }
        curr = curr->next;
    }
    return -1;
}

void Hash_Init(struct hash_t *H) {
    int i;
    for (i = 0; i < BUCKETS; i++) {
        List_Init(&H->lists[i]);
    }
}
int Hash_Insert(struct hash_t *H, char *key) {
    int hash_location = hashedString(key);
    return List_Insert(&H->lists[hash_location], key);
}

void Hash_Free(struct hash_t *H) {
    int i;
    for (i = 0; i < BUCKETS; i++)
        List_Free(&H->lists[i]);
}

int Hash_Lookup(struct hash_t *H, char *key) {
    int hash_location = hashedString(key);
    return List_Lookup(&H->lists[hash_location], key);
}

/*
  find the value of each character in the char string, and then give it
  an artificial weight by multiplying it with it's index in the string
*/
unsigned long hashedString(char *key) {
    unsigned long sum = 1;
    int ch, count = 0;
    while ((ch = *key++)) {
        sum += (count + 1) * ch;
        count += 1;
    }
    return sum % BUCKETS;
}

/* 
  initialize the queue
*/
void Queue_Init(struct queue_t *q) {
    struct node_t *node = malloc(sizeof(struct node_t));
    node->next = NULL;
    q->head = q->tail = node;
    pthread_mutex_init(&q->head_lock, NULL);
    pthread_mutex_init(&q->tail_lock, NULL);
}

/* 
  queue up the key to the end
*/
void Queue_Enqueue(struct queue_t *q, char *key) {
    struct node_t *new = malloc(sizeof(struct node_t));
    // create a new thread to store the value
    if (new == NULL) {
        perror("malloc");
        return;
    }
    // lock the head/tail as we don't want any other thread to access the queue
    // while we insert
    pthread_mutex_lock(&q->tail_lock);
    char *value = malloc(strlen(key) + 1);
    strcpy(value, key);
    new->key = value;
    new->next = NULL;
    q->tail->next = new;
    q->tail = new;
    pthread_mutex_unlock(&q->tail_lock);
}

int Queue_Dequeue(struct queue_t *q, char **value) {
    pthread_mutex_lock(&q->head_lock);
    struct node_t *curr = q->head;
    struct node_t *new_head = curr->next;

    if (new_head == NULL) {
        pthread_mutex_unlock(&q->head_lock);
        return -1;
    }

    *value = malloc(strlen(new_head->key) + 1);
    strcpy(*value, new_head->key);
    q->head = new_head;
    pthread_mutex_unlock(&q->head_lock);
    return 0;
}
/* 
  go through the given hash map and then see if the value is greater than or
  equal to the maximum in the queue right now.
  if it is greater that means that we need to dequeue all of the old dummy
  queue and then we need to enqueue the new value
  else if it is equal to the maximum value, we need to enqueue the new value
*/
struct max_t *Find_Max(struct hash_t *H) {
    int i, things_to_dequeue = 0, k;
    struct max_t *dummy_queue = malloc(sizeof(struct max_t));
    dummy_queue->words = malloc(sizeof(struct queue_t));
    dummy_queue->maximum = 0;
    Queue_Init(dummy_queue->words);
    for (i = 0; i < BUCKETS; i++) {
        struct node_t *curr = H->lists[i].head;
        while (curr) {
            if (curr->count > dummy_queue->maximum) {
                char *tmp;
                for (k = 0; k < things_to_dequeue; k++)
                {
                    Queue_Dequeue(dummy_queue->words, &tmp);
                }
                Queue_Enqueue(dummy_queue->words, curr->key);
                things_to_dequeue = 1;
                dummy_queue->maximum = curr->count;
            }
            else if (curr->count == dummy_queue->maximum) {
                Queue_Enqueue(dummy_queue->words, curr->key);
                things_to_dequeue++;
            }
            curr = curr->next; // keep iterating
        }
    }
    return dummy_queue;
}

