#ifndef PARSE_H
#define PARSE_H

#include "shell.h"

// Utility functions
char* trim_whitespace(char* str);
int is_empty_string(const char* str);
char** tokenize(const char* input, int* token_count);
void free_tokens(char** tokens);

// Parsing functions
Command* create_command();
RedirectionType get_redir_type(const char* token);
void parse_redirections(Command* cmd, char*** tokens_ptr, int* count_ptr);
int parse_input(const char* input, Command** cmd);

#endif