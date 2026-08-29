#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bedrock.h>


int main() {
    puts("== GLIBC MALLOC ==");
    char* victim = malloc(32);
    strcpy(victim, "SECRET_DATA");

    free(victim);

    char* replacement = malloc(32);
    strcpy(replacement, "ATTACKER_DATA");

    printf("victim: %p, replacement: %p\n", (void*)victim, (void*)replacement);
    printf("Secret contents: %s\n", victim);

    puts("== BEDROCK ALLOC ==");

    char* victim2 = bedrock_alloc(32);
    strcpy(victim2, "SECRET_DATA");

    bedrock_free(victim2);

    char* replacement2 = bedrock_alloc(32);
    strcpy(replacement2, "ATTACKER_DATA");

    printf("victim: %p, replacement: %p\n", (void*)victim2, (void*)replacement2);
    printf("Secret contents; %s\n", victim2);

    return 0;
}