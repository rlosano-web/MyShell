#ifndef LOGGER_H
#define LOGGER_H

#include "shell.h"

// Logging functions
void log_command(Shell* self, pid_t pid, const char* cmd_line, int status);

#endif