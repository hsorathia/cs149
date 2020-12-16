#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

// global variable needed to contain the pid of the child
// so the alarm can kill it
pid_t child;

// this is called when the alarm goes off
void alarm_handler(int sig) {
    kill(child, SIGKILL);
}


int main(int argc, char **argv) {
    // all the cases where the users input is wrong
    // if the user enters no number for max_tries or timeout 
    // atoi() of either of them will return 0
    if (argc < 4 || atoi(argv[1]) < 1 || atoi(argv[2]) < 1) {
        printf("USAGE: ./unflake max_tries max_timeout test_command args...\n");
        printf("max_tries - must be greater than or equal to 1\n");
        printf("max_timeout - number of seconds must be greater than or equal to 1\n");
        return 1;
    }
    // execargs are the extra arguments the user needs to run their tests
    int max_tries = atoi(argv[1]), max_timeout = atoi(argv[2]);
    char *execArgs[argc-2];
    for (int i = 3; i <= argc; i++) {
        if (i == argc) {
            execArgs[i-3] = NULL;
        } else {
            execArgs[i-3] = argv[i];
        }
    }

    // run     - tracks the amount of times we run the program the user wants us to
    // rc      - return code for the exec() if the rc gets set to anything then we know that
    //           program execution failed
    // sigRc   - if the signal is called, this will hold the value of the signal
    // wstatus - contains the status code for wait
    // fd      - file descriptor for creating new files (that we write input to)
    // exitCode- contains the exit/return code of the file that gets executed by the user
    // fileName     - contains the name of the file we're going to write to
    int run = 0, rc, sigRc, wstatus, fd, exitCode, stdOut = dup(1);
    char fileName[20];

    for (int i = 0; i < max_tries; i++) {
        // set an alarm signal, and create file name.
        // ensure that we're going to be writing to the file for the run
        signal(SIGALRM, alarm_handler);
        snprintf(fileName, sizeof(fileName), "test_output.%d", i+1);
        fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        fflush(stdout);
        dup2(fd, 1);
        dup2(fd, 2);
        
        run += 1;
        child = fork();
        // on the last run we should dump all our information to the console
        if (run == max_tries) {
            fflush(stdout);
            dup2(stdOut, 1);
        }
        if (child == 0) {
            // track the current run count, and run the file 
            printf("%d runs\n", run);
            rc = execvp(argv[3], execArgs);
            // if the execution fails, we note that down as failure and exit with code 2
            if (rc == -1) {
                printf("could not exec %s\n", argv[3]);
            }
            _exit(2);
        } else {

            // set an alarm with the timeout the user passed and wait for the child
            alarm(max_timeout);
            wait(&wstatus);

            // with the status of wait, we can check to see if the alarm was called or the child was killed
            // if the alarm was killed we record the signal number and report it and set the exit code to 255
            if (WIFSIGNALED(wstatus)) { // alarm called
                sigRc = WTERMSIG(wstatus);
                printf("killed with signal %d\n", sigRc);
                exitCode = 255;
            } else if (WIFEXITED(wstatus)) { // program executed correctly
                exitCode = WEXITSTATUS(wstatus);
            } else { // for some reason the program didn't execute correctly so we just return 255
                exitCode = 255;
            }

        }
    }

    // report the exit code and then return it
    if (exitCode != 255) {
        printf("exit code %d\n", exitCode);
    }
    return exitCode;
}