#ifndef SIGNALS_H
#define SIGNALS_H

#include "shell.h"

// Signal handling
void setup_signal_handlers();
void sigchld_handler(int sig);

#endif