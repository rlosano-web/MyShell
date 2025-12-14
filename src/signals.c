#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "signals.h"

// ==================== SIGNAL HANDLERS ====================
void sigchld_handler(int sig) {
    (void)sig;
    
    int status;
    pid_t pid;
    
    // Reap all zombie children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Child reaped successfully
    }
}

void setup_signal_handlers() {
    // Ignore SIGINT in parent
    signal(SIGINT, SIG_IGN);
    
    // Handle SIGCHLD to avoid zombies
    signal(SIGCHLD, sigchld_handler);
}