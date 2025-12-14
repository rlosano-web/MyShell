#ifndef EXECUTE_H
#define EXECUTE_H

#include "shell.h"

// Command lifecycle
void command_destroy(Command* cmd);

// Shell lifecycle
Shell* create_shell();
void destroy_shell(Shell* shell);
void shell_init(Shell* self);
void shell_cleanup(Shell* self);
void shell_run(Shell* self);

// Execution functions
void execute_command(Shell* self, Command* cmd);
int execute_external(Shell* self, Command* cmd);
int execute_pipeline(Shell* self, Command* cmd1, Command* cmd2);
int setup_redirections(Shell* self, Command* cmd);
void restore_std_fds(Shell* self);

#endif