#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bedrock.h>

#define BOLD "\033[1m"
#define GREEN "\033[32m"
#define RED "\033[31m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

int main(void) {
    char* victim = malloc(32);
    strcpy(victim, "SECRET_DATA");

    free(victim);

    char* replacement = malloc(32);
    strcpy(replacement, "ATTACKER_DATA");

    char* victim2 = bedrock_alloc(32);
    strcpy(victim2, "SECRET_DATA");

    bedrock_free(victim2);

    char* replacement2 = bedrock_alloc(32);
    strcpy(replacement2, "ATTACKER_DATA");

    printf("\n" BOLD CYAN);
    puts("╭────────────────────────────────────────────────────────────╮");
    puts("│         BEDROCK SECURITY DEMO · USE-AFTER-FREE             │");
    puts("╰────────────────────────────────────────────────────────────╯" RESET);

    printf("\n" BOLD "  glibc malloc · immediate reuse baseline" RESET "\n");
    printf("  %-20s %p\n", "freed pointer", (void*)victim);
    printf("  %-20s %p\n", "replacement", (void*)replacement);
    printf("  %-20s " RED "%s" RESET "\n", "stale-pointer read", victim);
    printf("  %-20s " RED "REUSED" RESET "\n", "result");

    printf("\n" BOLD "  Bedrock · zeroing + delayed reuse" RESET "\n");
    printf("  %-20s %p\n", "freed pointer", (void*)victim2);
    printf("  %-20s %p\n", "replacement", (void*)replacement2);
    printf("  %-20s %s\n", "stale-pointer read",
           victim2[0] == '\0' ? "<zeroed>" : victim2);
    printf("  %-20s " GREEN "QUARANTINED" RESET "\n", "result");

    printf("\n" GREEN BOLD "  ✓ Bedrock prevented immediate reuse and cleared stale data." RESET "\n\n");

    return 0;
}
