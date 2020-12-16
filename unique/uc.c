#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define BUCKETS 256
struct node_t {
    char* key;
    struct node_t *next;
};

struct list_t {
    struct node_t *head;
    pthread_mutex_t lock;
};

struct hash_t {
    struct list_t lists[BUCKETS];
};

struct thread_info {            
    pthread_t thread_id;        
    int       thread_num;      
    char      *argv_string;     
};

struct hash_t *H;

/*
  This function is called whenever a new thread is created. It tries to open a
  file and inserts the words (as long as they're unique) into our table.
*/
void *process_file(void *arg);
/* 
  This function is used to initialize the head nodes of all the lists.
*/
void List_Init(struct list_t *L);
/* 
  This function is used to insert a new key into the front of the list
*/
int List_Insert(struct list_t *L, char* key);
/* 
  This function is used to look up a value in the list. It returns -1 if the
  value is not found, and 0 if the value is found.
*/
int List_Lookup(struct list_t *L, char* key);
/* 
  This function is used to find the size of a given list.
*/
int List_Size(struct list_t *L);
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
int Hash_Insert(struct hash_t *H, unsigned long hash, char* key);
/* 
  This function looks up a value in the hashtable and returns if it was found or
  not.
*/
int Hash_Lookup(struct hash_t *H, unsigned long hash, char* key);
/* 
  This function is used to find the total size of a hashtable (when we count)
  at the end.
*/
int Hash_Size(struct hash_t *H);
/* 
  This function is used to iterate through the hashtable and free all the lists.
*/
void Hash_Free(struct hash_t *H);
/* 
  This function takes in a char string and calculate a hash value to store.
*/
unsigned long hashString(char *word);


int main(int argc, char **argv) {
    int count = 0;
    if (argc >= 2) {
        int i, s, amountOfThreads = argc-1;
        struct thread_info *thread;
        void *res;
        H = malloc(sizeof(struct hash_t));
        Hash_Init(H);
        thread = calloc(amountOfThreads, sizeof(struct thread_info));
        if (thread == NULL)
            perror("calloc");

        for (i = 0; i < amountOfThreads; i++) {
            // printf("thread %d\n", i);
            thread[i].thread_num = i+1;
            thread[i].argv_string = argv[i + 1];
            s = pthread_create(&thread[i].thread_id, NULL, &process_file, &thread[i]);
            if (s != 0) {
                printf("some error occurred");
                exit(2);
            }
        }
        
        for (i = 0; i < amountOfThreads; i++) {
            s = pthread_join(thread[i].thread_id, &res);
            if (s != 0) {
                printf("some error occurred");
                exit(2); 
            }
        }
        free(res);
        free(thread);
        count = Hash_Size(H);
        Hash_Free(H);
        free(H);
    }
    printf("%d\n", count);
    return 0;
}

void *process_file(void *arg) {
    struct thread_info *curr = arg;
    FILE *fh = fopen(curr->argv_string, "r");
    if (fh == NULL) {
        // printf("error opening file\n");
        perror(curr->argv_string);
    } else {
        char *ptr;
        int curr_sum;
        while (fscanf(fh, "%ms", &ptr) != EOF) {
            // printf("%s\n", ptr);
            curr_sum = hashString(ptr);
            Hash_Insert(H, curr_sum, ptr);
            free(ptr);
        }
        // printf("\n");
        free(ptr);
        fclose(fh);
    }

    return NULL;
}


void List_Init(struct list_t *L) {
    // &L = calloc(BUCKETS, sizeof(struct list_t));
    L->head = NULL;
    pthread_mutex_init(&L->lock, NULL);
}

/* 
  before inserting into the string, check to see if the string exists in the
  list or not. if it does then we don't save the value.
*/
int List_Insert(struct list_t *L, char* key) {
    // printf("%d %d\n", key, store);
    struct node_t *new = malloc(sizeof(struct node_t));
    if (new == NULL) {
        perror("malloc");
        return -1;
    }
    pthread_mutex_lock(&L->lock);
    if (List_Lookup(L, key) == 0) {
        free(new);
        pthread_mutex_unlock(&L->lock);
        return -1;
    }

    char* value = strdup(key);
    new->key = value;
    new->next = L->head;
    L->head = new;
    pthread_mutex_unlock(&L->lock);
    return 0;
}

int List_Lookup(struct list_t *L, char* key) {
    int return_value = -1;
    struct node_t *curr = L->head;
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            return_value = 0;
            break;
        }
        curr = curr->next;
    }
    return return_value;
}
int List_Size(struct list_t *L) {
    int count = 0;

    struct node_t *curr = L->head;
    while (curr) {
        count += 1;
        curr = curr->next;
    }
    return count;
}

void List_Free(struct list_t *L) {
    struct node_t *first, *second;
    first = L->head;
    second = L->head;
    while (first) {
        first = first->next;
        free(second->key);
        free(second);
        second = first;
    }
}

void Hash_Init(struct hash_t *H) {
    int i;
    for (i = 0; i < BUCKETS; i++) {
        List_Init(&H->lists[i]);
    }
}

int Hash_Insert(struct hash_t *H, unsigned long hash, char* key) {
    int hash_location = hash % BUCKETS;
    return List_Insert(&H->lists[hash_location], key);
}

int Hash_Lookup(struct hash_t *H, unsigned long hash, char* key) {
    int hash_location = hash % BUCKETS;
    return List_Lookup(&H->lists[hash_location], key);
}

int Hash_Size(struct hash_t *H) {
    int count = 0, i;
    for (i = 0; i < BUCKETS; i++) {
        count += List_Size(&H->lists[i]);
    }
    return count;

}

void Hash_Free(struct hash_t *H) {
    // printf("here\n");
    int i;
    for (i = 0; i < BUCKETS; i++) {
        List_Free(&H->lists[i]);
    }
}

/*
  find the value of each character in the char string, and then give it
  an artificial weight by multiplying it with it's index in the string
*/
unsigned long hashString(char *word) {
    unsigned long sum = 1;
    int ch, count = 0;
    while ((ch = *word++)) {
        sum += (count + 1) * ch;
        count += 1;
    }

    return sum;
}
