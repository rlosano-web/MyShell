#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "logger.h"

// ==================== LOGGING FUNCTIONS ====================
void log_command(Shell* self, pid_t pid, const char* cmd_line, int status) {
    if (!self || self->log_fd < 0 || !cmd_line) return;
    
    // Get current time
    time_t now = time(NULL);
    char time_buf[64];
    struct tm* tm_info = localtime(&now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Format log entry
    char log_entry[1024];
    int len = snprintf(log_entry, sizeof(log_entry),
                      "[%s] pid=%d cmd=\"%s\" status=%d\n",
                      time_buf, pid, cmd_line, status);
    
    if (len > 0) {
        write(self->log_fd, log_entry, len);
    }
}