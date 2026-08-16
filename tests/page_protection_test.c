// This primarily tests guard page behaviour, as well as UAF mitigations.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "utils.h"
#include "heap.h"

int main() {
    for (int i = 0 ; i < 10 ; i++) {
        int random_bytes = 2049 + (rand() % (64 * 1024 - 2049 +1));

        char* ptr = (char*)heap_alloc(random_bytes);
        pid_t pid = fork();
        if (pid == 0) {
            pid_t parent_pid = getppid();

            memset(ptr, 'A', round_to_nearest_page(random_bytes) + sysconf(_SC_PAGESIZE)); 
            printf("[BAD] gaurd page failed!\n");

            if (kill(parent_pid, SIGKILL) == -1) {
                perror("Kill failed");
                exit(0); 
            }

            exit(0);
        } else {
            int status;
            pid_t child_pid = waitpid(pid, &status, 0);

            if (child_pid == -1) {
                perror("[BAD] waitpid execution failed\n");
            }
        }
    }

    printf("[OK] Guard page tests passed.\n");

    for (int i = 0 ; i < 10 ; i++) {
        int random_bytes = 2049 + (rand() % (64 * 1024 - 2049 +1));

        char* ptr = (char*)heap_alloc(random_bytes);
        heap_free(ptr);
        pid_t pid = fork();
        if (pid == 0) {
            pid_t parent_pid = getppid();
            ptr[0] = 'A';
            printf("[BAD] UAF test failed!\n");

            if (kill(parent_pid, SIGKILL) == -1) {
                perror("Kill failed");
                exit(0); 
            }

            exit(0);
        } else {
            int status;
            pid_t child_pid = waitpid(pid, &status, 0);

            if (child_pid == -1) {
                perror("[BAD] waitpid execution failed\n");
            }
        }
    }

    printf("[OK] UAF tests passed.\n");
}