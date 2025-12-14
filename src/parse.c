#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"

// ==================== UTILITY FUNCTIONS ====================
char* trim_whitespace(char* str) {
    if (!str) return NULL;
    
    char* end;
    
    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }
    
    if (*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n')) {
        end--;
    }
    
    *(end + 1) = '\0';
    return str;
}

int is_empty_string(const char* str) {
    if (!str) return 1;
    
    while (*str) {
        if (*str != ' ' && *str != '\t' && *str != '\n') {
            return 0;
        }
        str++;
    }
    return 1;
}

char** tokenize(const char* input, int* token_count) {
    if (!input || is_empty_string(input)) {
        *token_count = 0;
        return NULL;
    }
    
    char* input_copy = strdup(input);
    if (!input_copy) return NULL;
    
    // Count tokens
    int count = 0;
    int in_token = 0;
    
    for (int i = 0; input_copy[i]; i++) {
        if (input_copy[i] != ' ' && input_copy[i] != '\t' && input_copy[i] != '\n') {
            if (!in_token) {
                count++;
                in_token = 1;
            }
        } else {
            in_token = 0;
        }
    }
    
    // Allocate array
    char** tokens = malloc((count + 1) * sizeof(char*));
    if (!tokens) {
        free(input_copy);
        return NULL;
    }
    
    // Tokenize
    int i = 0;
    char* token = strtok(input_copy, " \t\n");
    while (token) {
        tokens[i] = strdup(token);
        if (!tokens[i]) {
            // Cleanup on error
            for (int j = 0; j < i; j++) free(tokens[j]);
            free(tokens);
            free(input_copy);
            return NULL;
        }
        i++;
        token = strtok(NULL, " \t\n");
    }
    tokens[i] = NULL;
    
    *token_count = count;
    free(input_copy);
    return tokens;
}

void free_tokens(char** tokens) {
    if (!tokens) return;
    
    for (int i = 0; tokens[i]; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

// ==================== COMMAND CREATION ====================
Command* create_command() {
    Command* cmd = malloc(sizeof(Command));
    if (!cmd) return NULL;
    
    memset(cmd, 0, sizeof(Command));
    cmd->argv = NULL;
    cmd->argc = 0;
    cmd->background = 0;
    cmd->cmd_type = CMD_EXTERNAL;
    cmd->pipe_next = NULL;
    cmd->input_redir.type = REDIR_NONE;
    cmd->output_redir.type = REDIR_NONE;
    cmd->input_redir.filename = NULL;
    cmd->output_redir.filename = NULL;
    
    return cmd;
}

// ==================== REDIRECTION PARSING ====================
RedirectionType get_redir_type(const char* token) {
    if (!token) return REDIR_NONE;
    
    if (strcmp(token, "<") == 0) return REDIR_IN;
    if (strcmp(token, ">") == 0) return REDIR_OUT;
    if (strcmp(token, ">>") == 0) return REDIR_APPEND;
    return REDIR_NONE;
}

void parse_redirections(Command* cmd, char*** tokens_ptr, int* count_ptr) {
    if (!cmd || !tokens_ptr || !count_ptr) return;
    
    char** tokens = *tokens_ptr;
    int count = *count_ptr;
    
    for (int i = 0; i < count; i++) {
        RedirectionType type = get_redir_type(tokens[i]);
        if (type != REDIR_NONE) {
            if (i + 1 < count) {
                // Found redirection symbol with filename
                if (type == REDIR_IN) {
                    cmd->input_redir.type = type;
                    cmd->input_redir.filename = strdup(tokens[i + 1]);
                } else {
                    cmd->output_redir.type = type;
                    cmd->output_redir.filename = strdup(tokens[i + 1]);
                }
                
                // Remove redirection token and filename from array
                free(tokens[i]);
                free(tokens[i + 1]);
                
                // Shift remaining tokens
                for (int j = i; j < count - 2; j++) {
                    tokens[j] = tokens[j + 2];
                }
                count -= 2;
                tokens[count] = NULL;
                i--; // Adjust index
            }
        }
    }
    
    *tokens_ptr = tokens;
    *count_ptr = count;
}

// ==================== MAIN PARSING FUNCTION ====================
int parse_input(const char* input, Command** cmd) {
    if (!input || is_empty_string(input)) {
        return 0;
    }
    
    // Check for background job
    int background = 0;
    char* input_copy = strdup(input);
    trim_whitespace(input_copy);
    
    int len = strlen(input_copy);
    if (len > 0 && input_copy[len - 1] == '&') {
        background = 1;
        input_copy[len - 1] = '\0';
        trim_whitespace(input_copy);
    }
    
    // Check for pipe
    char* pipe_ptr = strchr(input_copy, '|');
    if (pipe_ptr) {
        // Split into two commands
        *pipe_ptr = '\0';
        char* first_part = trim_whitespace(input_copy);
        char* second_part = trim_whitespace(pipe_ptr + 1);
        
        if (is_empty_string(first_part) || is_empty_string(second_part)) {
            free(input_copy);
            return 0;
        }
        
        // Parse first command
        Command* cmd1 = create_command();
        if (!cmd1) {
            free(input_copy);
            return 0;
        }
        cmd1->background = background;
        
        int count1;
        char** tokens1 = tokenize(first_part, &count1);
        if (!tokens1) {
            free(cmd1);
            free(input_copy);
            return 0;
        }
        
        parse_redirections(cmd1, &tokens1, &count1);
        cmd1->argc = count1;
        cmd1->argv = tokens1;
        
        // Parse second command
        Command* cmd2 = create_command();
        if (!cmd2) {
            free_tokens(tokens1);
            free(cmd1);
            free(input_copy);
            return 0;
        }
        cmd2->background = background;
        
        int count2;
        char** tokens2 = tokenize(second_part, &count2);
        if (!tokens2) {
            free_tokens(tokens1);
            free(cmd1);
            free(cmd2);
            free(input_copy);
            return 0;
        }
        
        parse_redirections(cmd2, &tokens2, &count2);
        cmd2->argc = count2;
        cmd2->argv = tokens2;
        
        // Link them
        cmd1->pipe_next = cmd2;
        *cmd = cmd1;
        
        free(input_copy);
        return 1;
    }
    
    // No pipe - single command
    Command* command = create_command();
    if (!command) {
        free(input_copy);
        return 0;
    }
    command->background = background;
    
    int count;
    char** tokens = tokenize(input_copy, &count);
    if (!tokens) {
        free(command);
        free(input_copy);
        return 0;
    }
    
    parse_redirections(command, &tokens, &count);
    command->argc = count;
    command->argv = tokens;
    command->pipe_next = NULL;
    
    *cmd = command;
    free(input_copy);
    return 1;
}