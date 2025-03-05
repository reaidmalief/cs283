#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

# Basic Pipe Tests
@test "Pipe: ls | grep .c should show .c files" {
    run ./dsh <<EOF
ls | grep .c
EOF
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" == *".c" ]]
}

@test "Pipe: Multiple command pipe ls | sort | head works" {
    run ./dsh <<EOF
ls | sort | head -n 3
EOF
    [ "$status" -eq 0 ]
    [ "${#lines[@]}" -ge 1 ]
}

@test "Pipe: cat /etc/passwd | grep root works" {
    run ./dsh <<EOF
cat /etc/passwd | grep root
EOF
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" == *"root"* ]]
}

# Complex Pipe Scenarios
@test "Pipe: Multiple pipes with longer chain" {
    run ./dsh <<EOF
ls -l | grep .c | sort | wc -l
EOF
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" =~ ^[0-9]+$ ]]
}

@test "Pipe: Pipes with different commands" {
    run ./dsh <<EOF
echo "hello world" | wc -w
EOF
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "2" ]
}


@test "Pipe: Pipe with whitespace" {
    run ./dsh <<EOF
ls    |    grep .c
EOF
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" == *".c" ]]
}

@test "Pipe: Maximum number of pipes" {
  run ./dsh <<EOF
  ls | grep .c | sort | wc -l | cat | echo "test" | tr 'a-z' 'A-Z' | wc -c
  EOF
  [ "$status" -eq 0 ]
}

@test "Pipe: Built-in command in pipe" {
  run ./dsh <<EOF
  echo "test" | wc -l
  EOF
  [ "$status" -eq 0 ]
  [ "${lines[0]}" = "1" ]
}

# Complex Command Scenarios
@test "Pipe: Combination of commands with arguments" {
    run ./dsh <<EOF
ls -l | grep ^d | wc -l
EOF
    [ "$status" -eq 0 ]
    [[ "${lines[0]}" =~ ^[0-9]+$ ]]
}

@test "Pipe: Command with quoted arguments" {
    run ./dsh <<EOF
echo "test string" | wc -w
EOF
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "2" ]
}

# Performance and Reliability
@test "Pipe: Multiple rapid pipe executions" {
    run ./dsh <<EOF
ls | grep .c
ls | sort
ls | head -n 3
EOF
    [ "$status" -eq 0 ]
    [ "${#lines[@]}" -ge 3 ]
}

# Extra Credit: Input Redirection Tests
@test "Redirection: input redirection < works" {
    echo "test input file" > input.txt
    run ./dsh <<EOF
cat < input.txt
EOF
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "test input file" ]
    rm input.txt
}

@test "Redirection: output redirection > creates file" {
    run ./dsh <<EOF
echo "hello world" > output.txt
cat output.txt
EOF
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "hello world" ]
    rm output.txt
}

@test "Redirection: output redirection overwrites existing file" {
    echo "old content" > output.txt
    run ./dsh <<EOF
echo "new content" > output.txt
cat output.txt
EOF
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "new content" ]
    rm output.txt
}

@test "Redirection: append >> works" {
    echo "first line" > append.txt
    run ./dsh <<EOF
echo "second line" >> append.txt
cat append.txt
EOF
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "first line" ]
    [ "${lines[1]}" = "second line" ]
    rm append.txt
}

# Complex Redirection Scenarios
@test "Redirection: pipe with input redirection" {
    echo "apple
banana
cherry" > fruits.txt
    run ./dsh <<EOF
cat < fruits.txt | grep "an"
EOF
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "banana" ]
    rm fruits.txt
}

@test "Redirection: complex command with multiple redirections" {
    run ./dsh <<EOF
echo "test data" | tr ' ' '\n' > output.txt
cat output.txt
EOF
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "test" ]
    [ "${lines[1]}" = "data" ]
    rm output.txt
}


@test "Redirection: append preserves previous content" {
    echo "first line" > multi-line.txt
    run ./dsh <<EOF
echo "second line" >> multi-line.txt
echo "third line" >> multi-line.txt
cat multi-line.txt
EOF
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "first line" ]
    [ "${lines[1]}" = "second line" ]
    [ "${lines[2]}" = "third line" ]
    rm multi-line.txt
}