// #include <stdio.h>
// #include <stdlib.h>
// #include <stdarg.h>
// #include <fcntl.h>
// #include <pthread.h>
// #include <string.h>
// #include <unistd.h>


// int counter = 0;

// #define COUNTERS 3

// struct threads {
// 	pthread_t thread_id;
// 	int		  thread_num;
// 	int 	  ch;
// };

// void *mythread(void *arg) {
// 	struct threads *curr = arg;
// 	char ch = curr->ch;
// 	printf("%c: begin\n", ch);
// 	int i;
// 	for (i = 0; i < 1e7; i++) {
// 		counter = counter + 1;
// 	}
// 	printf("%c: done\n", ch);
// 	return NULL;
// }

// int main() {
// 	struct threads *my_thread;
// 	my_thread = calloc(COUNTERS, sizeof(struct threads));
// 	if (my_thread == NULL) {
// 		perror("calloc");
// 		return -1;
// 	}
// 	printf("main: begin (counter = %d)\n", counter);
// 	int i;
// 	for (i = 0; i < COUNTERS; i++) {
// 		my_thread[i].thread_num = i;
// 		my_thread[i].ch = ('A' + i);
// 		pthread_create(&my_thread[i].thread_id, NULL, mythread, &my_thread[i]);
// 	}

// 	for (i = 0; i < COUNTERS; i++) {
// 		pthread_join(my_thread[i].thread_id, NULL);
// 	}
// 	printf("main: end (counter = %d)\n", counter);
// 	return 1;
// }
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


static void lookup(const char *arg)
{
    DIR *dirp;
    struct dirent *dp;


    if ((dirp = opendir(".")) == NULL) {
        perror("couldn't open '.'");
        return;
    }


    do {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) {
            if (strcmp(dp->d_name, arg) != 0)
                continue;


            (void) printf("found %s\n", arg);
            (void) closedir(dirp);
                return;


        }
    } while (dp != NULL);


    if (errno != 0)
        perror("error reading directory");
    else
        (void) printf("failed to find %s\n", arg);
    (void) closedir(dirp);
    return;
}


int main(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc; i++)
        lookup(argv[i]);
    return (0);
}
