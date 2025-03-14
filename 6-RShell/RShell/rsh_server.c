#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <stdatomic.h>

//INCLUDES for extra credit
#include <signal.h>
#include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"


// Global variable for multi-threaded mode.
// If non-zero then the server spawns a new thread for each client.
int threaded_server = 0;

// Global variable to track last return code
static volatile int keep_running = 1;
static atomic_int shutdown_requested = 0;

static atomic_int next_client_id = 1;

// Signal handler to stop the server
void signal_handler(int sig) {
    (void)sig; // Avoid unused parameter warning
    keep_running = 0;
    atomic_store(&shutdown_requested, 1);
}


/*
 * set_threaded_server(int val)
 *
 *   sets the global flag to enable (non-zero)
 *   or disable (zero) multi-threaded server operation.
 */
void set_threaded_server(int val) {
    threaded_server = val;
}

/*
 * handle_client(void *arg)
 *
 *   Thread function that handles a client connection.
 *   'arg' is a pointer to the client socket descriptor.
 */
void *handle_client(void *arg) {
    int cli_socket = *(int *)arg;
    free(arg);  // Free the allocated memory for the socket descriptor
    
    // Assign each client a unique ID
    int my_id = atomic_fetch_add(&next_client_id, 1);

    // Process client requests
    int rc = exec_client_requests(cli_socket);

    // If the client sends `stop-server`
    if (rc == OK_EXIT) {
        printf("Thread %d received stop-server command\n", my_id);
        atomic_store(&shutdown_requested, 1);
        kill(getpid(), SIGINT); // Or raise(SIGINT), whichever you prefer
    }

    close(cli_socket);
    
    // At the end, print exactly what the BATS tests expect
    printf("Client %d done\n", my_id);
    return NULL;
}

/*
 * exec_client_thread(int main_socket, int cli_socket)
 *
 *   Spawns a new thread to handle a client connection.
 *   Returns OK if the thread was created successfully.
 */
int exec_client_thread(int main_socket, int cli_socket) {
    (void)main_socket;  // Unused parameter
    
    pthread_t tid;
    int *pclient = malloc(sizeof(int));
    if (!pclient) {
        perror("malloc");
        close(cli_socket);
        return ERR_RDSH_COMMUNICATION;
    }
    *pclient = cli_socket;
    if (pthread_create(&tid, NULL, handle_client, pclient) != 0) {
        perror("pthread_create");
        free(pclient);
        close(cli_socket);
        return ERR_RDSH_COMMUNICATION;
    }
    // Detach the thread so its resources are automatically freed when it exits.
    pthread_detach(tid);
    return OK;
}


/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:  a string in ip address format, indicating the interface
 *              where the server will bind.  In almost all cases it will
 *              be the default "0.0.0.0" which binds to all interfaces.
 *              note the constant RDSH_DEF_SVR_INTFACE in rshlib.h
 * 
 *      port:   The port the server will use.  Note the constant 
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.  If you are using
 *              tux you may need to change this to your own default, or even
 *              better use the command line override -s implemented in dsh_cli.c
 *              For example ./dsh -s 0.0.0.0:5678 where 5678 is the new port  
 * 
 *      is_threded:  Used for extra credit to indicate the server should implement
 *                   per thread connections for clients  
 * 
 *      This function basically runs the server by: 
 *          1. Booting up the server
 *          2. Processing client requests until the client requests the
 *             server to stop by running the `stop-server` command
 *          3. Stopping the server. 
 * 
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.  
 * 
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.  
 */
int start_server(char *ifaces, int port, int is_threaded) {
    int svr_socket;
    int rc;

    set_threaded_server(is_threaded);
    if (is_threaded) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);
    }

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        return svr_socket;
    }

    rc = process_cli_requests(svr_socket);
    stop_server(svr_socket);
    return (rc == OK_EXIT || atomic_load(&shutdown_requested)) ? OK : rc; // Normalize to OK for stop-server
}

/*
 * stop_server(svr_socket)
 *      svr_socket: The socket that was created in the boot_server()
 *                  function. 
 * 
 *      This function simply returns the value of close() when closing
 *      the socket.  
 */
int stop_server(int svr_socket){
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 *      ifaces & port:  see start_server for description.  They are passed
 *                      as is to this function.   
 * 
 *      This function "boots" the rsh server.  It is responsible for all
 *      socket operations prior to accepting client connections.  Specifically: 
 * 
 *      1. Create the server socket using the socket() function. 
 *      2. Calling bind to "bind" the server to the interface and port
 *      3. Calling listen to get the server ready to listen for connections.
 * 
 *      after creating the socket and prior to calling bind you might want to 
 *      include the following code:
 * 
 *      int enable=1;
 *      setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
 * 
 *      when doing development you often run into issues where you hold onto
 *      the port and then need to wait for linux to detect this issue and free
 *      the port up.  The code above tells linux to force allowing this process
 *      to use the specified port making your life a lot easier.
 * 
 *  Returns:
 * 
 *      server_socket:  Sockets are just file descriptors, if this function is
 *                      successful, it returns the server socket descriptor, 
 *                      which is just an integer.
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code is returned if the socket(),
 *                               bind(), or listen() call fails. 
 * 
 */
int boot_server(char *ifaces, int port){
    int svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    // Allow immediate reuse of the port
    int enable = 1;
    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ifaces);
    
    if (bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    /*
     * Prepare for accepting connections. The backlog size is set
     * to 20. So while one request is being processed other requests
     * can be waiting.
     */
    int ret = listen(svr_socket, 20);
    if (ret == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    printf("Server listening on %s:%d\n", ifaces, port);
    return svr_socket;
}

/*
 * process_cli_requests(svr_socket)
 *      svr_socket:  The server socket that was obtained from boot_server()
 *   
 *  This function handles managing client connections.  It does this using
 *  the following logic
 * 
 *      1.  Starts a while(1) loop:
 *  
 *          a. Calls accept() to wait for a client connection. Recall that 
 *             the accept() function returns another socket specifically
 *             bound to a client connection. 
 *          b. Calls exec_client_requests() to handle executing commands
 *             sent by the client. It will use the socket returned from
 *             accept().
 *          c. Loops back to the top (step 2) to accept connecting another
 *             client.  
 * 
 *          note that the exec_client_requests() return code should be
 *          negative if the client requested the server to stop by sending
 *          the `stop-server` command.  If this is the case step 2b breaks
 *          out of the while(1) loop. 
 * 
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to 
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket. 
 * 
 *  Returns:
 * 
 *      OK_EXIT:  When the client sends the `stop-server` command this function
 *                should return OK_EXIT. 
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code terminates the loop and is
 *                returned from this function in the case of the accept() 
 *                function failing. 
 * 
 *      OTHERS:   See exec_client_requests() for return codes.  Note that positive
 *                values will keep the loop running to accept additional client
 *                connections, and negative values terminate the server. 
 * 
 */
int process_cli_requests(int svr_socket) {
    int cli_socket;
    int rc = OK;

    while (keep_running && !atomic_load(&shutdown_requested)) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        // Accept a new client connection
        cli_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (cli_socket < 0) {
            if (errno == EINTR || atomic_load(&shutdown_requested)) {
                // Interrupted by signal_handler, exit gracefully
                printf("Server shutting down as requested\n");
                break;
            }
            perror("accept");
            rc = ERR_RDSH_COMMUNICATION;
            break;
        }

        char client_info[100];
        snprintf(client_info, sizeof(client_info), "Client connected: %s:%d\n", 
                inet_ntoa(client_addr.sin_addr), 
                ntohs(client_addr.sin_port));
        printf("%s", client_info);

        if (threaded_server) {
            // Handle client in a new thread
            if (exec_client_thread(svr_socket, cli_socket) != OK) {
                close(cli_socket);
                printf("Failed to create thread for client\n");
            }
        } else {
            // Handle client in the main thread
            rc = exec_client_requests(cli_socket);
            close(cli_socket);
            printf("Client disconnected. Return code: %d\n", rc);
            if (rc == OK_EXIT) {
                // Stop-server command received
                break;
            }
        }
    }

    return (rc == OK_EXIT || atomic_load(&shutdown_requested)) ? OK_EXIT : rc;
}
/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *   
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 * 
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection. 
 *      2.  When the client executes the `stop-server` command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop. 
 * 
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last 
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard. 
 * 
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish 
 *  this. 
 * 
 *  Of final note, this function must allocate a buffer for storage to 
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 * 
 *  Returns:
 * 
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client. 
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 * 
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors. 
 */
int exec_client_requests(int cli_socket) {
    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (!io_buff) return ERR_RDSH_SERVER;
    int overall_rc = OK;

    while (1) {
        int total_received = 0;
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);

        while (total_received < RDSH_COMM_BUFF_SZ) {
            ssize_t recvd = recv(cli_socket, io_buff + total_received, RDSH_COMM_BUFF_SZ - total_received, 0);
            if (recvd <= 0) {
                if (recvd < 0) {
                    perror("recv");
                    free(io_buff);
                    return ERR_RDSH_COMMUNICATION;
                }
                free(io_buff);
                return OK; // Client disconnect
            }
            total_received += recvd;
            if (memchr(io_buff, '\0', total_received) != NULL) break;
        }
        
        if (total_received >= RDSH_COMM_BUFF_SZ) {
            send_message_string(cli_socket, "Error: Command too large\n");
            send_message_eof(cli_socket);
            continue;
        }
        if (total_received == 0) {
            free(io_buff);
            return OK;
        }

        command_list_t cmd_list;
        memset(&cmd_list, 0, sizeof(command_list_t));
        int parse_rc = build_cmd_list(io_buff, &cmd_list);
        if (parse_rc != OK) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Error: Command parsing failed (%d)\n", parse_rc);
            send_message_string(cli_socket, err_msg);
            send_message_eof(cli_socket);
            goto cleanup_cmd_list;
        }

        // Handle built-in commands
        if (cmd_list.num == 1) {
            Built_In_Cmds builtin = rsh_built_in_cmd(&cmd_list.commands[0], cli_socket);
            if (builtin == BI_CMD_EXIT) {
                send_message_string(cli_socket, "Exiting client connection...\n");
                send_message_eof(cli_socket);
                overall_rc = OK;
                break;
            }
            if (builtin == BI_CMD_STOP_SVR) {
                send_message_string(cli_socket, "Stopping server...\n");
                send_message_eof(cli_socket);
                if (threaded_server) {
                    atomic_store(&shutdown_requested, 1);
                    keep_running = 0;
                    kill(getpid(), SIGTERM);
                }
                overall_rc = OK_EXIT;
                break;
            }
            if (builtin == BI_CMD_DRAGON) {
                print_dragon();
                send_message_eof(cli_socket);
                goto cleanup_cmd_list;
            }
            if (builtin == BI_CMD_CD) {
                if (cmd_list.commands[0].argc > 1) {
                    if (chdir(cmd_list.commands[0].argv[1]) != 0) {
                        char err_msg[256];
                        snprintf(err_msg, sizeof(err_msg), "cd: %s: %s\n", 
                                cmd_list.commands[0].argv[1], strerror(errno));
                        send_message_string(cli_socket, err_msg);
                    } else {
                        char cwd[1024];
                        if (getcwd(cwd, sizeof(cwd)) != NULL) {
                            char success_msg[1280];
                            snprintf(success_msg, sizeof(success_msg), "Changed directory to: %s\n", cwd);
                            send_message_string(cli_socket, success_msg);
                        }
                    }
                }
                send_message_eof(cli_socket);
                goto cleanup_cmd_list;
            }
        }

        // Validate command existence before execution
        if (cmd_list.commands[0].argv[0][0] != '/' && !strchr(cmd_list.commands[0].argv[0], '/')) {
            char *path = getenv("PATH");
            char *path_copy = strdup(path ? path : "");
            char *path_part;
            int cmd_found = 0;
            
            for (path_part = strtok(path_copy, ":"); path_part; path_part = strtok(NULL, ":")) {
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", path_part, cmd_list.commands[0].argv[0]);
                if (access(full_path, X_OK) == 0) {
                    cmd_found = 1;
                    break;
                }
            }
            free(path_copy);
            
            if (!cmd_found) {
                char err_msg[256];
                snprintf(err_msg, sizeof(err_msg), "Error: Command '%s' not found\n", 
                        cmd_list.commands[0].argv[0]);
                send_message_string(cli_socket, err_msg);
                send_message_eof(cli_socket);
                goto cleanup_cmd_list;
            }
        }

        // Execute pipeline
        int exec_rc = rsh_execute_pipeline(cli_socket, &cmd_list);
        if (exec_rc != 0) {
            char rc_msg[64];
            snprintf(rc_msg, sizeof(rc_msg), "Command returned %d\n", exec_rc);
            send_message_string(cli_socket, rc_msg);
        }
        send_message_eof(cli_socket);

    cleanup_cmd_list:
        for (int i = 0; i < cmd_list.num; i++) {
            if (cmd_list.commands[i].input_redirect) free(cmd_list.commands[i].input_redirect);
            if (cmd_list.commands[i].output_redirect) free(cmd_list.commands[i].output_redirect);
            free_cmd_buff(&cmd_list.commands[i]);
        }
    }

    free(io_buff);
    return overall_rc;
}

/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent. 
 * 
 *  Returns:
 * 
 *      OK:  The EOF character was sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character. 
 */
int send_message_eof(int cli_socket){
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len;
    sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);

    if (sent_len != send_len){
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}


/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client. 
 *   
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated. 
 * 
 *  Returns:
 * 
 *      OK:  The message in buff followed by the EOF character was 
 *           sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character. 
 */
int send_message_string(int cli_socket, char *buff){
    //TODO implement writing to cli_socket with send()
    int total_sent = 0;
    int len = strlen(buff);
    // Loop to ensure the entire message is sent.
    while (total_sent < len) {
        int sent = send(cli_socket, buff + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            perror("send");
            return ERR_RDSH_COMMUNICATION;
        }
        total_sent += sent;
    }
    // After sending the message, send the EOF marker.
    return send_message_eof(cli_socket);
}


/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell. 
 *   
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable. 
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:  
 * 
 *      
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │    
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │    
 *      │   │              │     │              │     │              │  │  │    
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘  │    
 *          │              │ │   │              │ │   │              │     │    
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘    
 *          └──────────────┘     └──────────────┘     └──────────────┘   
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function       
 * 
 *  Returns:
 * 
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value. 
 */
int rsh_execute_pipeline(int cli_socket, command_list_t *clist) {
    int num = clist->num;
    int pipes[num - 1][2];
    pid_t pids[num];
    int exit_code = 0;

    // Create all necessary pipes
    for (int i = 0; i < num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Error creating pipe: %s\n", strerror(errno));
            send_message_string(cli_socket, err_msg);
            return ERR_EXEC_CMD;
        }
    }

    // Create all processes
    for (int i = 0; i < num; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Error forking process: %s\n", strerror(errno));
            send_message_string(cli_socket, err_msg);
            
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_EXEC_CMD;
        }
        
        if (pids[i] == 0) { // Child process
            // Set up stdin
            if (clist->commands[i].input_redirect) {
                int fd_in = open(clist->commands[i].input_redirect, O_RDONLY);
                if (fd_in < 0) {
                    char err_msg[256];
                    snprintf(err_msg, sizeof(err_msg), "Cannot open input file: %s\n", strerror(errno));
                    write(cli_socket, err_msg, strlen(err_msg));
                    _exit(EXIT_FAILURE);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            } else if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            // Set up stdout and stderr
            if (clist->commands[i].output_redirect) {
                int flags = O_WRONLY | O_CREAT | (clist->commands[i].append_redirect ? O_APPEND : O_TRUNC);
                int fd_out = open(clist->commands[i].output_redirect, flags, 0644);
                if (fd_out < 0) {
                    char err_msg[256];
                    snprintf(err_msg, sizeof(err_msg), "Cannot open output file: %s\n", strerror(errno));
                    write(cli_socket, err_msg, strlen(err_msg));
                    _exit(EXIT_FAILURE);
                }
                dup2(fd_out, STDOUT_FILENO);
                dup2(fd_out, STDERR_FILENO);
                close(fd_out);
            } else {
                // Always direct output to cli_socket unless redirected
                if (i == num - 1 || num == 1) {
                    dup2(cli_socket, STDOUT_FILENO);
                    dup2(cli_socket, STDERR_FILENO);
                } else {
                    dup2(pipes[i][1], STDOUT_FILENO);
                    dup2(pipes[i][1], STDERR_FILENO);
                }
            }

            // Close unused pipe ends in child
            for (int j = 0; j < num - 1; j++) {
                if (j != i - 1) close(pipes[j][0]);  // Keep read end open if it's our input
                if (j != i) close(pipes[j][1]);      // Keep write end open if it's our output
            }

            // Execute the command
            Built_In_Cmds builtin = rsh_built_in_cmd(&clist->commands[i], cli_socket);
            if (builtin == BI_NOT_BI) {
                execvp(clist->commands[i].argv[0], clist->commands[i].argv);
                char err_msg[256];
                snprintf(err_msg, sizeof(err_msg), "Error executing %s: %s\n", 
                        clist->commands[i].argv[0], strerror(errno));
                write(cli_socket, err_msg, strlen(err_msg));
                _exit(127);
            } else if (builtin == BI_CMD_DRAGON) {
                print_dragon();
                _exit(0);
            }
            _exit(0);
        }
    }

    // Parent process
    for (int i = 0; i < num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes and ensure output is flushed
    for (int i = 0; i < num; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == num - 1) {
            if (WIFEXITED(status)) {
                exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                exit_code = 128 + WTERMSIG(status);
            } else {
                exit_code = 1;
            }
        }
    }

    return exit_code;
}

/**************   OPTIONAL STUFF  ***************/
/****
 **** NOTE THAT THE FUNCTIONS BELOW ALIGN TO HOW WE CRAFTED THE SOLUTION
 **** TO SEE IF A COMMAND WAS BUILT IN OR NOT.  YOU CAN USE A DIFFERENT
 **** STRATEGY IF YOU WANT.  IF YOU CHOOSE TO DO SO PLEASE REMOVE THESE
 **** FUNCTIONS AND THE PROTOTYPES FROM rshlib.h
 **** 
 */

/*
 * rsh_match_command(const char *input)
 *      cli_socket:  The string command for a built-in command, e.g., dragon,
 *                   cd, exit-server
 *   
 *  This optional function accepts a command string as input and returns
 *  one of the enumerated values from the BuiltInCmds enum as output. For
 *  example:
 * 
 *      Input             Output
 *      exit              BI_CMD_EXIT
 *      dragon            BI_CMD_DRAGON
 * 
 *  This function is entirely optional to implement if you want to handle
 *  processing built-in commands differently in your implementation. 
 * 
 *  Returns:
 * 
 *      BI_CMD_*:   If the command is built-in returns one of the enumeration
 *                  options, for example "cd" returns BI_CMD_CD
 * 
 *      BI_NOT_BI:  If the command is not "built-in" the BI_NOT_BI value is
 *                  returned. 
 */
Built_In_Cmds rsh_match_command(const char *input)
{
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    if (strcmp(input, "stop-server") == 0)
        return BI_CMD_STOP_SVR;
    if (strcmp(input, "rc") == 0)
        return BI_CMD_RC;
    return BI_NOT_BI;
}

/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 *      cmd:  The cmd_buff_t of the command, remember, this is the 
 *            parsed version fo the command
 *   
 *  This optional function accepts a parsed cmd and then checks to see if
 *  the cmd is built in or not.  It calls rsh_match_command to see if the 
 *  cmd is built in or not.  Note that rsh_match_command returns BI_NOT_BI
 *  if the command is not built in. If the command is built in this function
 *  uses a switch statement to handle execution if appropriate.   
 * 
 *  Again, using this function is entirely optional if you are using a different
 *  strategy to handle built-in commands.  
 * 
 *  Returns:
 * 
 *      BI_NOT_BI:   Indicates that the cmd provided as input is not built
 *                   in so it should be sent to your fork/exec logic
 *      BI_EXECUTED: Indicates that this function handled the direct execution
 *                   of the command and there is nothing else to do, consider
 *                   it executed.  For example the cmd of "cd" gets the value of
 *                   BI_CMD_CD from rsh_match_command().  It then makes the libc
 *                   call to chdir(cmd->argv[1]); and finally returns BI_EXECUTED
 *      BI_CMD_*     Indicates that a built-in command was matched and the caller
 *                   is responsible for executing it.  For example if this function
 *                   returns BI_CMD_STOP_SVR the caller of this function is
 *                   responsible for stopping the server.  If BI_CMD_EXIT is returned
 *                   the caller is responsible for closing the client connection.
 * 
 *   AGAIN - THIS IS TOTALLY OPTIONAL IF YOU HAVE OR WANT TO HANDLE BUILT-IN
 *   COMMANDS DIFFERENTLY. 
 */
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd, int cli_socket) {
    Built_In_Cmds ctype = BI_NOT_BI;
    ctype = rsh_match_command(cmd->argv[0]);

    switch (ctype) {
        case BI_CMD_DRAGON:
            print_dragon();
            return BI_EXECUTED;
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;
        case BI_CMD_STOP_SVR:
            return BI_CMD_STOP_SVR;
        case BI_CMD_RC:
            return BI_CMD_RC;
        case BI_CMD_CD:
            if (cmd->argc > 1) {
                if (chdir(cmd->argv[1]) != 0) {
                    char err_msg[256];
                    snprintf(err_msg, sizeof(err_msg), "cd: %s: %s\n", cmd->argv[1], strerror(errno));
                    send_message_string(cli_socket, err_msg);
                } else {
                    char cwd[1024];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        char success_msg[1280];
                        snprintf(success_msg, sizeof(success_msg), "Changed directory to: %s\n", cwd);
                        send_message_string(cli_socket, success_msg);
                    }
                }
            } else {
                send_message_string(cli_socket, "cd: missing argument\n");
            }
            return BI_EXECUTED;
        default:
            return BI_NOT_BI;
    }
}