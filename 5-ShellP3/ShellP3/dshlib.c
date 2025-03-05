#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include "dshlib.h"

// Global variable to track last return code
static int last_return_code = 0;

// Helper function to trim whitespace
static char* trim(char* str) 
{
    if (!str) return NULL;
    
    // Trim leading spaces
    while (isspace(*str)) str++;
    
    if (*str == 0) return str;
    
    // Trim trailing spaces
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    end[1] = '\0';
    
    return str;
}

// Allocate memory for command buffer
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (!cmd_buff) return ERR_CMD_OR_ARGS_TOO_BIG;
    
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = NULL;
    cmd_buff->input_redirect = NULL;
    cmd_buff->output_redirect = NULL;
    cmd_buff->append_redirect = false;
    
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

// Free memory for command buffer
int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (!cmd_buff) return ERR_CMD_OR_ARGS_TOO_BIG;
    
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    
    cmd_buff->input_redirect = NULL;
    cmd_buff->output_redirect = NULL;
    cmd_buff->append_redirect = false;
    
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    cmd_buff->argc = 0;
    
    return OK;
}

// Clear command buffer
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    free_cmd_buff(cmd_buff);
    return alloc_cmd_buff(cmd_buff);
}

// Build command buffer from command string
int build_cmd_buff(char *input, cmd_buff_t *cmd)
{
    if (!input || !cmd) return ERR_CMD_OR_ARGS_TOO_BIG;

    // Always start clean
    clear_cmd_buff(cmd);

    // Skip leading/trailing whitespace
    input = trim(input);
    if (*input == '\0') {
        return WARN_NO_CMDS;
    }

    cmd->_cmd_buffer = strdup(input);
    if (!cmd->_cmd_buffer) {
        return ERR_MEMORY;
    }

    char *curr = cmd->_cmd_buffer;
    char *token = curr;
    int argc = 0;

    bool in_quotes = false;
    char quote_char = '\0';
    bool input_redirect_parsed = false;
    bool output_redirect_parsed = false;

    while (*curr) {
        // Handle quote tracking
        if (*curr == '"' || *curr == '\'') {
            if (!quote_char) {
                // Start of quote
                quote_char = *curr;
                memmove(curr, curr + 1, strlen(curr));
                in_quotes = true;
                continue;
            } 
            else if (*curr == quote_char) {
                // End of quote
                memmove(curr, curr + 1, strlen(curr));
                in_quotes = false;
                quote_char = '\0';
                continue;
            }
        }

        // Skip processing redirections inside quotes
        if (in_quotes) {
            curr++;
            continue;
        }

        // Input redirection handling
        if (*curr == '<' && !input_redirect_parsed) {
            *curr = '\0';
            token = trim(token);
            
            // Save command arguments before input redirect
            if (token != curr && *token) {
                if (argc >= CMD_ARGV_MAX - 1) {
                    fprintf(stderr, "error: too many arguments\n");
                    return ERR_TOO_MANY_COMMANDS;
                }
                cmd->argv[argc++] = token;
            }

            // Move to next token after '<'
            curr++;
            while (isspace((unsigned char)*curr)) curr++;
            
            // Find end of input redirect filename
            char *filename_start = curr;
            while (*curr && !isspace((unsigned char)*curr)) curr++;
            
            // Null-terminate filename
            if (*curr) {
                *curr = '\0';
                curr++;
            }

            // Store input redirect
            cmd->input_redirect = strdup(filename_start);
            input_redirect_parsed = true;
            token = curr;
            continue;
        }

        // Output redirection handling
        if (*curr == '>' && !output_redirect_parsed) {
            *curr = '\0';
            token = trim(token);
            
            // Save command arguments before output redirect
            if (token != curr && *token) {
                if (argc >= CMD_ARGV_MAX - 1) {
                    fprintf(stderr, "error: too many arguments\n");
                    return ERR_TOO_MANY_COMMANDS;
                }
                cmd->argv[argc++] = token;
            }

            // Check for append redirection
            if (*(curr + 1) == '>') {
                cmd->append_redirect = true;
                curr++;
            }

            // Move to next token after '>'
            curr++;
            while (isspace((unsigned char)*curr)) curr++;
            
            // Find end of output redirect filename
            char *filename_start = curr;
            while (*curr && !isspace((unsigned char)*curr)) curr++;
            
            // Null-terminate filename
            if (*curr) {
                *curr = '\0';
                curr++;
            }

            // Store output redirect
            cmd->output_redirect = strdup(filename_start);
            output_redirect_parsed = true;
            token = curr;
            continue;
        }

        // Normal argument processing
        if (!in_quotes && isspace((unsigned char)*curr)) {
            *curr = '\0';
            
            if (token != curr && *token) {
                if (argc >= CMD_ARGV_MAX - 1) {
                    fprintf(stderr, "error: too many arguments\n");
                    return ERR_TOO_MANY_COMMANDS;
                }
                cmd->argv[argc++] = token;
            }
            
            token = curr + 1;
        }
        
        curr++;
    }

    // Handle last token
    if (token != curr && *token) {
        if (argc >= CMD_ARGV_MAX - 1) {
            fprintf(stderr, "error: too many arguments\n");
            return ERR_TOO_MANY_COMMANDS;
        }
        cmd->argv[argc++] = token;
    }

    cmd->argv[argc] = NULL;
    cmd->argc = argc;

    return OK;
}

// Build command list from input line
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (!cmd_line || !clist) return ERR_CMD_OR_ARGS_TOO_BIG;
    
    // Clear existing command list
    memset(clist, 0, sizeof(command_list_t));
    
    // Trim the command line
    cmd_line = trim(cmd_line);
    
    // Check for empty command or only pipe characters
    if (strlen(cmd_line) == 0 || strspn(cmd_line, " |") == strlen(cmd_line)) {
        fprintf(stderr, "Error: Invalid pipe configuration\n");
        return ERR_CMD_ARGS_BAD;
    }
    
    // Check for consecutive pipes
    char *prev = NULL;
    char *curr = cmd_line;
    while ((curr = strstr(curr, PIPE_STRING)) != NULL) {
        if (prev && curr - prev <= 1) {
            fprintf(stderr, "Error: Consecutive or empty pipes\n");
            return ERR_CMD_ARGS_BAD;
        }
        prev = curr;
        curr++;
    }
    
    // Split commands by pipe
    char *cmd_str = strtok(cmd_line, PIPE_STRING);
    while (cmd_str && clist->num < CMD_MAX) {
        // Trim each command segment
        cmd_str = trim(cmd_str);
        
        // Skip empty command segments
        if (strlen(cmd_str) == 0) {
            fprintf(stderr, "Error: Empty command in pipe\n");
            return ERR_CMD_ARGS_BAD;
        }
        
        cmd_buff_t *curr_cmd = &clist->commands[clist->num];
        
        // Reset current command buffer
        clear_cmd_buff(curr_cmd);
        
        // Parse command into cmd_buff_t
        int parse_rc = build_cmd_buff(cmd_str, curr_cmd);
        if (parse_rc != OK) {
            return parse_rc;
        }
        
        clist->num++;
        cmd_str = strtok(NULL, PIPE_STRING);
    }
    
    // Check if too many commands
    if (cmd_str) {
        fprintf(stderr, "Error: Too many commands in pipeline\n");
        return ERR_TOO_MANY_COMMANDS;
    }
    
    // Ensure at least one valid command
    if (clist->num == 0) {
        fprintf(stderr, "Error: No valid commands in pipeline\n");
        return ERR_CMD_ARGS_BAD;
    }
    
    return OK;
}

// Execute pipeline of commands
int execute_pipeline(command_list_t *clist)
{
    if (!clist || clist->num == 0) {
        fprintf(stderr, "Error: No commands to execute\n");
        return ERR_CMD_ARGS_BAD;
    }

    int  pipes[CMD_MAX - 1][2];
    pid_t child_pids[CMD_MAX];
    int any_command_failed = 0;  // Track if any command fails

    // Create pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            // Close any previously opened pipes
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_EXEC_CMD;
        }
    }

    // For each command in the pipeline, fork/exec
    for (int i = 0; i < clist->num; i++) {
        child_pids[i] = fork();
        if (child_pids[i] < 0) {
            perror("fork");
            // Close all pipes
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            // Kill already-forked children
            for (int j = 0; j < i; j++) {
                kill(child_pids[j], SIGTERM);
            }
            return ERR_EXEC_CMD;
        }

        if (child_pids[i] == 0) {
            // Child process
            // INPUT redirection
            if (clist->commands[i].input_redirect) {
                int fd_in = open(clist->commands[i].input_redirect, O_RDONLY);
                if (fd_in < 0) {
                    perror("open for input");
                    _exit(errno);
                }
                if (dup2(fd_in, STDIN_FILENO) < 0) {
                    perror("dup2 input");
                    close(fd_in);
                    _exit(errno);
                }
                close(fd_in);
            }
            else if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2 pipe input");
                    _exit(errno);
                }
            }

            // OUTPUT redirection
            if (clist->commands[i].output_redirect) {
                int flags = O_WRONLY | O_CREAT | 
                            (clist->commands[i].append_redirect ? O_APPEND : O_TRUNC);
                int fd_out = open(clist->commands[i].output_redirect, flags, 0644);
                if (fd_out < 0) {
                    perror("open for output");
                    _exit(errno);
                }
                if (dup2(fd_out, STDOUT_FILENO) < 0) {
                    perror("dup2 output");
                    close(fd_out);
                    _exit(errno);
                }
                close(fd_out);
            }
            else if (i < clist->num - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 pipe output");
                    _exit(errno);
                }
            }

            // Close all pipe FDs in child
            for (int p = 0; p < clist->num - 1; p++) {
                close(pipes[p][0]);
                close(pipes[p][1]);
            }

            // Execute command
            Built_In_Cmds builtin = exec_built_in_cmd(&clist->commands[i]);
            if (builtin == BI_NOT_BI) { 
                if (execvp(clist->commands[i].argv[0], clist->commands[i].argv) == -1) {
                    fprintf(stderr, "Error: Command '%s' not found\n", clist->commands[i].argv[0]);
                    perror("execvp");
                    _exit(127);
                }
            }

            _exit(0);
        }
    }

    // Parent process
    // Close all pipes
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait on each child
    int last_status = 0;
    for (int i = 0; i < clist->num; i++) {
        int wstatus = 0;
        waitpid(child_pids[i], &wstatus, 0);

        if (WIFEXITED(wstatus)) {
            int child_code = WEXITSTATUS(wstatus);
            if (child_code != 0) {
                any_command_failed = 1;
                if (last_status == 0) {
                    last_status = child_code;
                }
            }
        } 
        else if (WIFSIGNALED(wstatus)) {
            int sig_code = 128 + WTERMSIG(wstatus);
            any_command_failed = 1;
            if (last_status == 0) {
                last_status = sig_code;
            }
        }
    }

    last_return_code = any_command_failed ? last_status : 0;
    return last_return_code;
}


// Helper function for built-in commands
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    if (!cmd || cmd->argc == 0) {
        return BI_NOT_BI;
    }
    
    if (strcmp(cmd->argv[0], EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }
    else if (strcmp(cmd->argv[0], "dragon") == 0) {
        print_dragon();
        return BI_CMD_DRAGON;
    }
    else if (strcmp(cmd->argv[0], "cd") == 0) {
        // Implement cd built-in
        if (cmd->argc > 1) {
            if (chdir(cmd->argv[1]) != 0) {
                perror("cd");
                last_return_code = errno;
            } else {
                last_return_code = 0;
            }
        }
        return BI_CMD_CD;
    }
    
    return BI_NOT_BI;
}



/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */
int exec_local_cmd_loop()
{
    char input[SH_CMD_MAX];
    command_list_t clist;
    
    while (1) {
        // Clear the command list to prevent potential leftover data
        memset(&clist, 0, sizeof(command_list_t));
        
        // Print prompt and get input
        printf("%s", SH_PROMPT);
        
        // Handle input reading with error checking
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // Handle EOF (Ctrl+D)
            printf("\n");
            break;
        }
        
        // Remove trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }
        
        // Skip empty lines
        if (input[0] == '\0') {
            continue;
        }
        
        // Check for exit command
        if (strcmp(input, EXIT_CMD) == 0) {
            printf("exiting...\n");
            return last_return_code;
        }
        
        // Parse command list
        int parse_rc = build_cmd_list(input, &clist);
        
        // Handle parsing errors
        switch (parse_rc) {
            case WARN_NO_CMDS:
                printf(CMD_WARN_NO_CMD);
                continue;
            
            case ERR_TOO_MANY_COMMANDS:
                printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
                continue;
            
            case OK:
                break;
            
            default:
                fprintf(stderr, "Unexpected command parsing error\n");
                continue;
        }
        
        // Special handling for single built-in commands
        if (clist.num == 1) {
            Built_In_Cmds builtin = exec_built_in_cmd(&clist.commands[0]);
            if (builtin != BI_NOT_BI) {
                // Built-in command handled in parent
                continue;
            }
        }
        
        // Execute pipeline
        int exec_rc = execute_pipeline(&clist);
        
        // Handle execution errors
        if (exec_rc != OK) {
            fprintf(stderr, "Command execution error\n");
        }
        
        // Free resources for each command in the list
        for (int i = 0; i < clist.num; i++) {
            // Free input and output redirect strings
            if (clist.commands[i].input_redirect) {
                free(clist.commands[i].input_redirect);
                clist.commands[i].input_redirect = NULL;
            }
            
            if (clist.commands[i].output_redirect) {
                free(clist.commands[i].output_redirect);
                clist.commands[i].output_redirect = NULL;
            }
            
            // Clear the command buffer
            free_cmd_buff(&clist.commands[i]);
        }
    }
    
    return last_return_code;
}