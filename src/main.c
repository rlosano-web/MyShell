#include <stdio.h>
#include <stdlib.h>
#include "shell.h"
#include "execute.h"

int main(int argc, char** argv) {
    (void)argc; // Unused
    (void)argv; // Unused
    
    printf("=== MyShell - A Mini Unix Shell for CS-12600 ===\n");
    
    // Create shell instance
    Shell* shell = create_shell();
    if (!shell) {
        fprintf(stderr, "Failed to create shell\n");
        return 1;
    }
    
    // Initialize shell
    shell_init(shell);
    
    // Run shell main loop
    shell_run(shell);
    
    // Cleanup
    destroy_shell(shell);
    
    printf("Shell terminated. Goodbye!\n");
    return 0;
}
