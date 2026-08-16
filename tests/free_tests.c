#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include "heap.h"

// Generates a pseudo-random 64-bit unsigned integer
uint64_t rand64(void) {
    return ((uint64_t)rand() & 0xFFFF) << 48 |
           ((uint64_t)rand() & 0xFFFF) << 32 |
           ((uint64_t)rand() & 0xFFFF) << 16 |
           ((uint64_t)rand() & 0xFFFF);
}

int main() {

    srand((unsigned int)time(NULL));

    for (int i = 0; i < 10; i++)
        heap_free(NULL);

    printf("[OK] NULL free test passed.\n");

    for (size_t i = 0; i <= 2048; i++) {
        void* ptr = heap_alloc(i);
        heap_free(ptr);

        pid_t pid = fork();
        if (pid == 0) {
            pid_t parent_pid = getppid();
            heap_free(ptr);
            printf("[BAD] Small double free test failed.\n");

            if (kill(parent_pid, SIGKILL) == -1) {
                perror("Kill failed");
                exit(0); 
            }

            exit(0);
        } else {
            int status;
            pid_t child_pid = waitpid(pid, &status, 0);

            if (child_pid == -1) {
                perror("\t[BAD] waitpid execution failed\n");
            }
        }
    }

    printf("[OK] Small double free tests passed.\n");

    for (int i = 0 ; i < 1000 ; i++) {
        int random_bytes = 2049 + (rand() % (64 * 1024 - 2049 +1));
        void* ptr = heap_alloc(random_bytes);
        heap_free(ptr);

        pid_t pid = fork(); 
        if (pid == 0) {
            pid_t parent_pid = getppid();
            heap_free(ptr);
            perror("[BAD] Large double free test failed.\n");

            if (kill(parent_pid, SIGKILL) == -1) {
                perror("Kill failed");
                exit(0); 
            }
            exit(0);
        } else {
            int status;
            pid_t child_pid = waitpid(pid, &status, 0);

            if (child_pid == -1) {
                perror("\t[BAD] waitpid execution failed\n");
            }
        }
    }

    printf("[OK] Large double free tests passed.\n");

    for (size_t i = 1; i <= 2048; i++) {
        void* ptr = heap_alloc(i);

        void* random_ptr = (void*)((char*)ptr + ((rand() % i) + 1));
    
        pid_t pid = fork();
        if (pid == 0) {
            pid_t parent_pid = getppid();
            heap_free(random_ptr);
            printf("[BAD] Small double free test failed.\n");

            if (kill(parent_pid, SIGKILL) == -1) {
                perror("Kill failed");
                exit(0); 
            }
            exit(0);
        } else {
            int status;
            pid_t child_pid = waitpid(pid, &status, 0);

            if (child_pid == -1) {
                perror("\t[BAD] waitpid execution failed\n");
            }

            //heap_free(ptr);
        }

    }
    printf("[OK] Small interior pointer free tests passed.\n");

    for (int i = 0 ; i < 10 ; i++) {
        int random_bytes = 2049 + (rand() % (64 * 1024 - 2049 +1));
        void* ptr = heap_alloc(random_bytes);

        void* random_ptr =  (void*)((char*)ptr + ((rand() % random_bytes) + 1));

        pid_t pid = fork(); 
        if (pid == 0) {
            pid_t parent_pid = getppid();
            heap_free(random_ptr);
            perror("[BAD] Large double free test failed.\n");

            if (kill(parent_pid, SIGKILL) == -1) {
                perror("Kill failed");
                exit(0); 
            }
            exit(0);
        } else {
            int status;
            pid_t child_pid = waitpid(pid, &status, 0);

            if (child_pid == -1) {
                perror("\t[BAD] waitpid execution failed\n");
            }

        }
    }

    printf("[OK] Large interior pointer free tests passed.\n");

    for (int i = 0 ; i < 1000 ; i++) {
        pid_t pid = fork(); 
        if (pid == 0) {
            pid_t parent_pid = getppid();
            heap_free((void*)rand64());
            perror("[BAD] Large double free test failed.\n");

            if (kill(parent_pid, SIGKILL) == -1) {
                perror("Kill failed");
                exit(0); 
            }
            exit(0);
        } else {
            int status;
            pid_t child_pid = waitpid(pid, &status, 0);

            if (child_pid == -1) {
                perror("\t[BAD] waitpid execution failed\n");
            }
        }
    }

    printf("[OK] Arbitrary pointer free test passed.\n");

}