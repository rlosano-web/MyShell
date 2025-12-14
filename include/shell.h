#ifndef SHELL_H
#define SHELL_H

#include <stddef.h>
#include <sys/types.h>

// ==================== ENUMS ====================
typedef enum {
    REDIR_NONE,
    REDIR_IN,      // <
    REDIR_OUT,     // >
    REDIR_APPEND   // >>
} RedirectionType;

typedef enum {
    CMD_EXTERNAL,
    CMD_BUILTIN
} CommandType;

// ==================== FORWARD DECLARATIONS ====================
typedef struct Shell Shell;
typedef struct Command Command;
typedef struct Redirection Redirection;

// ==================== STRUCT DEFINITIONS ====================
// Redirection structure
struct Redirection {
    RedirectionType type;
    char* filename;
};

// Command structure
struct Command {
    char** argv;           // Array de argumentos
    int argc;             // Número de argumentos
    CommandType cmd_type;  // Tipo de comando
    int background;       // 1 si es trabajo en segundo plano
    
    struct Redirection input_redir;
    struct Redirection output_redir;
    struct Command* pipe_next;  // Siguiente comando en pipe
};

// Shell state structure
struct Shell {
    int running;
    int log_fd;
    int saved_stdin;
    int saved_stdout;
};

// ==================== FUNCTION DECLARATIONS ====================
// Factory functions (ahora en parse.h y execute.h)
// Utility functions (ahora en parse.h)

#endif