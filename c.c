#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>

int main() {
    char bruh[20];
    int fd[2];
    pipe(fd);
    int rc = fork();
    if (rc == 0) {
        close(fd[0]);
        write(fd[1], "bruh\n", 20);
        _exit(0);
    }
    else {
        int x = wait(NULL);
        close(fd[1]);
        read(fd[0], &bruh, 4);
    }
    

    printf("\nprinting bruh: %s\n", bruh);
    return 0;
}