#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */


// Helper function to trim leading and trailing spaces
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

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
        if (!cmd_line || !clist) return ERR_CMD_OR_ARGS_TOO_BIG;
    
    // Initialize the command list
    memset(clist, 0, sizeof(command_list_t));
    
    // Trim the command line
    cmd_line = trim(cmd_line);
    
    // Check for empty command
    if (strlen(cmd_line) == 0) {
        return WARN_NO_CMDS;
    }
    
    // Split commands by pipe
    char *cmd_str = strtok(cmd_line, PIPE_STRING);
    while (cmd_str) {
        // Check command limit
        if (clist->num >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }
        
        // Get current command structure
        command_t *curr_cmd = &clist->commands[clist->num];
        
        // Trim the command string
        cmd_str = trim(cmd_str);
        
        // Split into command and arguments
        char *space_pos = strchr(cmd_str, SPACE_CHAR);
        if (space_pos) {
            // We have arguments
            size_t cmd_len = space_pos - cmd_str;
            if (cmd_len >= EXE_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            
            // Copy command name
            strncpy(curr_cmd->exe, cmd_str, cmd_len);
            curr_cmd->exe[cmd_len] = '\0';
            
            // Copy arguments (skip leading space)
            space_pos++;
            if (strlen(space_pos) >= ARG_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strcpy(curr_cmd->args, trim(space_pos));
        } else {
            // No arguments
            if (strlen(cmd_str) >= EXE_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strcpy(curr_cmd->exe, cmd_str);
        }
        
        clist->num++;
        cmd_str = strtok(NULL, PIPE_STRING);
    }
    
    return OK;
}