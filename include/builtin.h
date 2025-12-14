#ifndef BUILTINS_H
#define BUILTINS_H

#include "shell.h"

// Builtin command function pointer type
typedef int (*BuiltinFunc)(Command* cmd);

// Builtin command structure
typedef struct {
    char* name;
    BuiltinFunc func;
} BuiltinCommand;

// Builtin functions
int builtin_cd(Command* cmd);
int builtin_exit(Command* cmd);
int builtin_pwd(Command* cmd);
int builtin_help(Command* cmd);

// Builtin registry
BuiltinCommand* get_builtin(const char* name);
int is_builtin_command(Command* cmd);
int execute_builtin(Command* cmd);

#endif