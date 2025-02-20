#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"
#include <errno.h>

// Global Variable to store last returen code
static int last_return_code = 0;

// Helper function to allocate memory for command buffer
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = NULL;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

// Helper function to free memory for command buffer
int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->argc = 0;
    return OK;
}

// Helper function to clear command buffer
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    free_cmd_buff(cmd_buff);
    return alloc_cmd_buff(cmd_buff);
}

// Helper function to handle command parsing
static int parse_command(char *input, cmd_buff_t *cmd) 
{
    char *token;
    bool in_quotes = false;
    int i = 0;

    // CLear previous command
    clear_cmd_buff(cmd);

    // Skip leading whitespace
    while (*input && isspace(*input)) {
        input++;
    }

    if (*input == '\0') {
        return WARN_NO_CMDS;
    }

    cmd->_cmd_buffer = strdup(input);
    if (!cmd->_cmd_buffer) {
        return ERR_MEMORY;
    }

    //Parse considering quotes
    char *curr = cmd->_cmd_buffer;
    token = curr;

    while (*curr) {
        if (*curr == '"') {
            in_quotes = !in_quotes;
            memmove(curr, curr + 1, strlen(curr));
            continue;
        }

        if (!in_quotes && isspace(*curr)) {
            *curr = '\0';
            if (token != curr) {
                if (i >= CMD_ARGV_MAX - 1) {
                    return ERR_TOO_MANY_COMMANDS;
                }
                cmd->argv[i++] = token;
            }
            token = curr + 1;
        }
        curr++;
    }

    if (token != curr && *token) {
        if (i >= CMD_ARGV_MAX - 1) {
            return ERR_TOO_MANY_COMMANDS;
        }
        cmd->argv[i++] = token;
    }

    cmd->argv[i] = NULL;
    cmd->argc = i;

    return OK;

}

// Handle built-in commands
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) 
{
    if (!cmd->argc) {
        return BI_NOT_BI;
    }

    if (strcmp(cmd->argv[0], EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc > 1) {
            if (chdir(cmd->argv[1]) != 0) {
                perror("cd");
                last_return_code = errno;
            } else {
                last_return_code = 0;
            }
        }
        return BI_CMD_CD;

    } else if (strcmp(cmd->argv[0], "rc") == 0) {
        printf("%d\n", last_return_code);
        return BI_RC;
    } else if (strcmp(cmd->argv[0], "dragon") == 0) {
        print_dragon();
        return BI_CMD_DRAGON;
    }
    return BI_NOT_BI;

}

// Function to handle execvp errors
void handle_exec_error() 
{
    switch (errno) {
        case ENOENT:
            fprintf(stderr, "Command not found in PATH\n");
            break;
        case EACCES:
            fprintf(stderr, "Permission denied\n");
            break;
        case ENOMEM:
            fprintf(stderr, "Out of memory\n");
            break;
        case E2BIG:
            fprintf(stderr, "Argument list too long\n");
            break;
        default:
            fprintf(stderr, "Execution error: %s\n", strerror(errno));
    }
    exit(errno);
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
    cmd_buff_t cmd = {0};
    int status;

    // TODO IMPLEMENT MAIN LOOP

    // TODO IMPLEMENT parsing input to cmd_buff_t *cmd_buff

    // TODO IMPLEMENT if built-in command, execute builtin logic for exit, cd (extra credit: dragon)
    // the cd command should chdir to the provided directory; if no directory is provided, do nothing

    // TODO IMPLEMENT if not built-in command, fork/exec as an external command
    // for example, if the user input is "ls -l", you would fork/exec the command "ls" with the arg "-l"

    if (alloc_cmd_buff(&cmd) != OK) {
        return ERR_MEMORY;
    }

    while (1) {
        printf("%s", SH_PROMPT);

        if (fgets(input, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        status = parse_command(input, &cmd);
        if (status == WARN_NO_CMDS){
            continue;
        } else if (status != OK) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }

        Built_In_Cmds builtin = exec_built_in_cmd(&cmd);
        if (builtin == BI_CMD_EXIT) {
            break;
        } else if (builtin != BI_NOT_BI) {
            continue;
        } 

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            last_return_code = errno;
            continue;
        } else if (pid == 0) {
            // Child process
            execvp(cmd.argv[0], cmd.argv);
            handle_exec_error();
        } else {
            // Parent process
            int wait_status;
            waitpid(pid, &wait_status, 0);

            if (WIFEXITED(wait_status)) {
                last_return_code = WEXITSTATUS(wait_status);
            } else if (WIFSIGNALED(wait_status)) {
                last_return_code = 128 + WTERMSIG(wait_status);
            }
        }
    }

    clear_cmd_buff(&cmd);
    return OK;
}
