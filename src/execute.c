#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "execute.h"
#include "builtin.h"
#include "logger.h"
#include "signals.h"
#include "parse.h"

// ==================== COMMAND LIFECYCLE ====================
void command_destroy(Command* cmd) {
    if (!cmd) return;
    
    // Free argv
    if (cmd->argv) {
        free_tokens(cmd->argv);
    }
    
    // Free redirection filenames
    if (cmd->input_redir.filename) {
        free(cmd->input_redir.filename);
    }
    if (cmd->output_redir.filename) {
        free(cmd->output_redir.filename);
    }
    
    // Free piped command
    if (cmd->pipe_next) {
        command_destroy(cmd->pipe_next);
    }
    
    free(cmd);
}

// ==================== SHELL LIFECYCLE ====================
Shell* create_shell() {
    Shell* shell = malloc(sizeof(Shell));
    if (!shell) return NULL;
    
    memset(shell, 0, sizeof(Shell));
    return shell;
}

void destroy_shell(Shell* shell) {
    if (!shell) return;
    
    shell_cleanup(shell);
    free(shell);
}

void shell_init(Shell* self) {
    if (!self) return;
    
    self->running = 1;
    self->log_fd = -1;
    self->saved_stdin = -1;
    self->saved_stdout = -1;
    
    // Initialize logger
    self->log_fd = open("myshell.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (self->log_fd < 0) {
        perror("open log file");
    }
    
    // Setup signal handlers
    setup_signal_handlers();
}

void shell_cleanup(Shell* self) {
    if (!self) return;
    
    if (self->log_fd >= 0) {
        close(self->log_fd);
        self->log_fd = -1;
    }
}

void shell_run(Shell* self) {
    if (!self) return;
    
    char input[1024];
    
    while (self->running) {
        // Display prompt
        printf("myshell> ");
        fflush(stdout);
        
        // Read input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // EOF (Ctrl-D) or error
            printf("\n");
            break;
        }
        
        // Trim and check for empty input
        char* trimmed_input = trim_whitespace(input);
        if (is_empty_string(trimmed_input)) {
            continue;
        }
        
        // Parse command
        Command* cmd = NULL;
        if (parse_input(trimmed_input, &cmd)) {
            if (cmd) {
                execute_command(self, cmd);
                command_destroy(cmd);
            }
        }
    }
}

// ==================== REDIRECTION HANDLING ====================
int setup_redirections(Shell* self, Command* cmd) {
    if (!self || !cmd) return -1;
    
    // Save original file descriptors
    self->saved_stdin = dup(STDIN_FILENO);
    self->saved_stdout = dup(STDOUT_FILENO);
    
    // Setup input redirection
    if (cmd->input_redir.type == REDIR_IN && cmd->input_redir.filename) {
        int fd = open(cmd->input_redir.filename, O_RDONLY);
        if (fd < 0) {
            perror("open input");
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 input");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    // Setup output redirection
    if (cmd->output_redir.type == REDIR_OUT && cmd->output_redir.filename) {
        int fd = open(cmd->output_redir.filename, 
                     O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open output");
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 output");
            close(fd);
            return -1;
        }
        close(fd);
    } else if (cmd->output_redir.type == REDIR_APPEND && cmd->output_redir.filename) {
        int fd = open(cmd->output_redir.filename,
                     O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) {
            perror("open append");
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 append");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    return 0;
}

void restore_std_fds(Shell* self) {
    if (!self) return;
    
    if (self->saved_stdin != -1) {
        dup2(self->saved_stdin, STDIN_FILENO);
        close(self->saved_stdin);
        self->saved_stdin = -1;
    }
    if (self->saved_stdout != -1) {
        dup2(self->saved_stdout, STDOUT_FILENO);
        close(self->saved_stdout);
        self->saved_stdout = -1;
    }
}

// ==================== COMMAND EXECUTION ====================
int execute_external(Shell* self, Command* cmd) {
    if (!self || !cmd || !cmd->argv || cmd->argc == 0) return -1;
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) { // Child process
        // Restore default SIGINT handler (in child)
        signal(SIGINT, SIG_DFL);
        
        // Setup redirections
        if (setup_redirections(self, cmd) < 0) {
            exit(EXIT_FAILURE);
        }
        
        // Execute command
        execvp(cmd->argv[0], cmd->argv);
        
        // If execvp returns, there was an error
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    
    // Parent process
    if (!cmd->background) {
        // Foreground job - wait for completion
        int status;
        waitpid(pid, &status, 0);
        
        // Build command line for logging
        char cmd_line[1024] = {0};
        for (int i = 0; i < cmd->argc; i++) {
            if (i > 0) strcat(cmd_line, " ");
            strcat(cmd_line, cmd->argv[i]);
        }
        
        int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        log_command(self, pid, cmd_line, exit_status);
        
        return exit_status;
    } else {
        // Background job
        printf("[bg] started pid %d\n", pid);
        return 0;
    }
}

int execute_pipeline(Shell* self, Command* cmd1, Command* cmd2) {
    if (!self || !cmd1 || !cmd2) return -1;
    
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return -1;
    }
    
    // First command (writes to pipe)
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    
    if (pid1 == 0) {
        // Child process 1
        signal(SIGINT, SIG_DFL);
        
        // Redirect stdout to pipe write end
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
            perror("dup2 pipe write");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        
        // Setup any redirections for cmd1
        if (setup_redirections(self, cmd1) < 0) {
            exit(EXIT_FAILURE);
        }
        
        execvp(cmd1->argv[0], cmd1->argv);
        perror("execvp cmd1");
        exit(EXIT_FAILURE);
    }
    
    // Second command (reads from pipe)
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    
    if (pid2 == 0) {
        // Child process 2
        signal(SIGINT, SIG_DFL);
        
        // Redirect stdin from pipe read end
        close(pipefd[1]);
        if (dup2(pipefd[0], STDIN_FILENO) < 0) {
            perror("dup2 pipe read");
            exit(EXIT_FAILURE);
        }
        close(pipefd[0]);
        
        // Setup any redirections for cmd2
        if (setup_redirections(self, cmd2) < 0) {
            exit(EXIT_FAILURE);
        }
        
        execvp(cmd2->argv[0], cmd2->argv);
        perror("execvp cmd2");
        exit(EXIT_FAILURE);
    }
    
    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);
    
    // Wait for both children
    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    
    // Build command line for logging
    char cmd_line1[512] = {0};
    char cmd_line2[512] = {0};
    
    for (int i = 0; i < cmd1->argc; i++) {
        if (i > 0) strcat(cmd_line1, " ");
        strcat(cmd_line1, cmd1->argv[i]);
    }
    
    for (int i = 0; i < cmd2->argc; i++) {
        if (i > 0) strcat(cmd_line2, " ");
        strcat(cmd_line2, cmd2->argv[i]);
    }
    
    char full_cmd[2048];
    int written = snprintf(full_cmd, sizeof(full_cmd), "%s | %s", cmd_line1, cmd_line2);
    if (written >= sizeof(full_cmd)) {
        full_cmd[sizeof(full_cmd) - 1] = '\0';
    }
    
    int exit_status = WIFEXITED(status2) ? WEXITSTATUS(status2) : -1;
    log_command(self, pid2, full_cmd, exit_status);
    
    return exit_status;
}

void execute_command(Shell* self, Command* cmd) {
    if (!self || !cmd) return;
    
    if (is_builtin_command(cmd)) {
        execute_builtin(cmd);
    } else if (cmd->pipe_next) {
        execute_pipeline(self, cmd, cmd->pipe_next);
    } else {
        execute_external(self, cmd);
    }
}