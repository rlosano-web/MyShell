#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "shell.h"
#include "builtin.h"

// ==================== BUILTIN IMPLEMENTATIONS ====================
int builtin_cd(Command* cmd) {
    const char* path;
    
    if (!cmd || cmd->argc == 1) {
        // No argument, go to HOME
        path = getenv("HOME");
        if (!path) {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    } else if (cmd->argc == 2) {
        path = cmd->argv[1];
    } else {
        fprintf(stderr, "cd: too many arguments\n");
        return 1;
    }
    
    if (chdir(path) != 0) {
        perror("cd");
        return 1;
    }
    
    return 0;
}

int builtin_exit(Command* cmd) {
    // Exit with status if provided
    int status = 0;
    if (cmd && cmd->argc > 1) {
        status = atoi(cmd->argv[1]);
    }
    exit(status);
    return status; // Not reached
}

int builtin_pwd(Command* cmd) {
    (void)cmd; // Unused
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd");
        return 1;
    }
}

int builtin_help(Command* cmd) {
    (void)cmd; // Unused
    
    printf("MyShell - A Mini Unix Shell\n");
    printf("Built-in commands:\n");
    printf("  cd [dir]      - Change directory\n");
    printf("  exit [status] - Exit shell\n");
    printf("  quit          - Exit shell\n");
    printf("  pwd           - Print working directory\n");
    printf("  help          - Show this help\n");
    printf("\n");
    printf("Features:\n");
    printf("  - External commands: ls, grep, etc.\n");
    printf("  - I/O redirection: >, >>, <\n");
    printf("  - Pipes: cmd1 | cmd2\n");
    printf("  - Background jobs: cmd &\n");
    
    return 0;
}

// ==================== BUILTIN REGISTRY ====================
static BuiltinCommand builtins[] = {
    {"cd", builtin_cd},
    {"exit", builtin_exit},
    {"quit", builtin_exit},
    {"pwd", builtin_pwd},
    {"help", builtin_help},
    {NULL, NULL}
};

BuiltinCommand* get_builtin(const char* name) {
    if (!name) return NULL;
    
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(builtins[i].name, name) == 0) {
            return &builtins[i];
        }
    }
    
    return NULL;
}

int is_builtin_command(Command* cmd) {
    if (!cmd || !cmd->argv || cmd->argc == 0) return 0;
    
    return get_builtin(cmd->argv[0]) != NULL;
}

int execute_builtin(Command* cmd) {
    if (!cmd || !cmd->argv || cmd->argc == 0) return 0;
    
    BuiltinCommand* builtin = get_builtin(cmd->argv[0]);
    if (!builtin) return 0;
    
    return builtin->func(cmd);
}