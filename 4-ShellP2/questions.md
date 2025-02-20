1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  We use `fork` before calling `execvp` because it lets the shell keep running after executing an external command. If we called execvp directly, it would replace the shell process entirely with the new command. Forking creates a child process to run the command, leaving the parent (shell) intact so that it can continue accepting new input.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  In my implementation, if fork() fails—usually because of resource limits—it returns -1. I check for that negative return value and then use perror("fork") to print an error message. I also update my global return code with errno so that the error is recorded, and then I simply continue the loop instead of crashing the shell.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**:  execvp() searches for the command in the directories listed in the PATH environment variable. If you provide a command without an absolute path, execvp() will go through each directory specified in PATH until it finds an executable that matches the command name.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**:  We call wait() in the parent process to ensure that it pauses until the child process finishes. This lets us capture the exit status of the child and also prevents zombie processes, processes that have finished but haven’t been properly “reaped” by their parent, from piling up. Without waiting, the parent might finish its loop and start new commands while old child processes are still lingering in the background, which isn’t ideal.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**:  We use WEXITSTATUS() to extract the actual exit code from the child process after it finishes. This exit code is important because it tells us if the command ran successfully (usually a 0) or if it encountered an error (a non-zero value). This information is crucial for things like our `rc` built-in command, which reports the status back to the user.

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**:  Although my function is called parse_command, it does the job of building the command buffer. I iterate through the input string and toggle an “in_quotes” flag whenever I hit a quotation mark. This lets me keep spaces inside quotes intact rather than treating them as argument separators. It’s necessary because without this, a command like echo "hello world" would split “hello” and “world” into separate arguments, which is not what I want.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**:  I refactored my parsing logic by moving from handling multiple commands with pipes to working with a single command buffer. I improved the way I trim extra spaces and handle quoted strings. The most unexpected challenge was getting the quote-handling right—making sure that spaces inside quotes aren’t removed, while extra spaces outside are properly trimmed. It took a few iterations, but I eventually got it to work as expected.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**:  In my understanding, signals are like quick notifications that tell a process something important happened (like an interrupt or termination request). They’re asynchronous and don’t carry data—they just alert the process to take a specific action. This is different from other IPC methods like pipes or shared memory, which are designed to exchange data between processes rather than just sending a simple “heads-up.”

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  SIGKILL: I see this as the “do it now” signal that forcefully kills a process. It can’t be caught or ignored, so it’s used when a process must be terminated immediately.  SIGTERM: This is a polite way to ask a process to stop. A process can catch SIGTERM to perform cleanup before exiting, so it’s used for graceful shutdowns.  SIGINT: I usually encounter this when I press Ctrl+C in the terminal. It interrupts a process and gives it a chance to clean up before terminating, if it’s set up to handle it.

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**:  When a process receives SIGSTOP, it is immediately suspended (paused) by the operating system. Unlike SIGINT, SIGSTOP cannot be caught, blocked, or ignored. This is by design because SIGSTOP is meant to force a process to stop execution without any chance for the process to interfere, which is useful for debugging or system management purposes.
