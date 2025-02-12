1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  fgets() is actually perfect for a shell because it reads input line by line, which is exactly how shell commands work! Unlike other functions like scanf() (which I've used before and can be messy), fgets() waits for the user to hit Enter before processing the input. This is super important because users might need to backspace and correct typos while typing their commands.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  So this is interesting - while we technically could have used a fixed-size array, malloc() is the better choice here for a few reasons. First, we're dealing with a potentially large buffer (ARG_MAX + EXE_MAX), and putting that on the stack with a fixed array could cause stack overflow issues, especially if our shell needs to handle recursive operations later.

3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**:  Trimming spaces is SUPER important! Think about it - if someone types "   ls   -l   " (with extra spaces), they obviously mean "ls -l". If we didn't trim those spaces, we'd literally be looking for a command called "   ls" (with spaces), which obviously doesn't exist! This would cause commands to fail even when they're technically correct. It would also mess up argument parsing and pipe handling, since spaces around the pipe symbol would interfere with command separation. Just like how we expect search engines to ignore extra spaces, users expect their shell to handle whitespace intelligently.

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  Three key redirection features we should implement are output to file (>), input from file (<), and error redirection (2>). The main challenges would be handling file permissions correctly, managing file existence checks, and properly separating different types of output streams. We'd also need to decide how to handle edge cases like when files don't exist or when users don't have proper permissions.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**:  While both handle I/O, redirection and piping serve different purposes. Redirection connects commands with files - either saving output to a file or reading input from one. Piping, on the other hand, creates a direct connection between two commands, where the output of one becomes the input of another. Think of redirection as file storage and piping as direct command-to-command communication.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  Separating STDERR from STDOUT is essential for practical shell usage. When redirecting output to a file, users typically want only the actual results, not error messages. This separation also makes error handling in scripts much more manageable since you can capture and process errors separately from normal output. Without this separation, it would be much harder to automate tasks and handle errors programmatically.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:  Our shell should keep STDOUT and STDERR separate by default but provide an option to merge them (like bash's 2>&1). This gives users flexibility while maintaining clean output separation for most use cases. We could implement this through file descriptor duplication, though we'd need to carefully handle edge cases. Starting with basic separation would be smart, then add merging capabilities once the core functionality is solid.