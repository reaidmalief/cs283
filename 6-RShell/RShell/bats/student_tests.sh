#!/usr/bin/env bats
# File: student_tests.sh
#
# Test suite for the Remote Drexel Shell (dsh) implementation.
#

# Setup and teardown to clean up any running server processes.
setup() {
  pkill -f "./dsh -s" || true
  sleep 1
}

teardown() {
  pkill -f "./dsh -s" || true
  sleep 1
}

# Helper to start the server on a given port.
start_server() {
    local port="$1"
    local threaded="$2"
    local threaded_flag=""
    [ "$threaded" = true ] && threaded_flag="-x"
    ./dsh -s -p "$port" $threaded_flag > /tmp/server_output_$port.txt 2>&1 &
    SERVER_PID=$!
    echo "$SERVER_PID" > "/tmp/server_pid_$port"
    sleep 1  # Wait for server to start
}

# Helper to stop the server by sending the "stop-server" command.
stop_server() {
    local port="$1"
    if [ -f "/tmp/server_pid_$port" ]; then
        local pid=$(cat "/tmp/server_pid_$port")
        if kill -0 "$pid" 2>/dev/null; then
            kill -TERM "$pid"
            wait "$pid" 2>/dev/null || true  # Ignore wait failure
        fi
        rm -f "/tmp/server_pid_$port" "/tmp/server_output_$port.txt"
    fi
}

# Helper to run a client command and capture its output.
# The command is piped to the client.
run_client_command() {
  local command="$1"
  echo "$command" | ./dsh -c -i 127.0.0.1 -p "$TEST_PORT"
}

# A small helper to remove lines that might apperar in the output 
# which can break the test cases:
#   - "local mode"
#   - "socket client mode: ..."
#   - "socket server mode: ..."
#   - "dsh4>"
#   - "exiting..."
#   - "cmd loop returned 0"
filter_dsh_noise() {
  echo "$1" \
    | sed '/^local mode/d' \
    | sed '/^socket client mode:/d' \
    | sed '/^socket server mode:/d' \
    | sed '/^-> Single-Threaded Mode/d' \
    | sed '/^-> Multi-Threaded Mode/d' \
    | sed '/^dsh4>/d' \
    | sed '/^exiting\.\.\./d' \
    | sed '/^cmd loop returned/d' \
    | sed '/^Server listening on/d' \
    | sed '/^Client connected:/d' \
    | sed '/^Client disconnected./d'
}

# -----------------------------
# Local Mode Tests
# -----------------------------

@test "Local: Basic command execution" {
  run bash -c "echo 'echo TEST' | ./dsh"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"TEST"* ]]
}

@test "Local: Command with arguments" {
  run bash -c "echo 'echo Hello World' | ./dsh"
  [ "$status" -eq 0 ]

  # Filter out dsh noise lines, then check for Hello World
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"Hello World"* ]]
}

@test "Local: Exit command" {
  run bash -c "echo 'exit' | ./dsh"
  [ "$status" -eq 0 ]
}

@test "Local: Built-in cd command" {
  run bash -c "echo -e 'cd /tmp\npwd\nexit' | ./dsh"
  [ "$status" -eq 0 ]

  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"/tmp"* ]]
}

@test "Local: Command with pipe" {
  run bash -c "echo 'echo dshlib.h file | grep dsh' | ./dsh"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"dshlib.h"* ]]
}


# -----------------------------
# Remote (Client-Server) Mode Tests
# -----------------------------

# Remote Mode Tests
TEST_PORT=5555

@test "Server: Start and stop server" {
  start_server "$TEST_PORT" false
  stop_server "$TEST_PORT"
  run pgrep -f "./dsh -s -p $TEST_PORT"
  [ "$status" -ne 0 ]
}

@test "Server: Multiple client connections" {
  TEST_PORT=5556
  start_server "$TEST_PORT" false
  
  run bash -c "echo 'echo Client 1' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"Client 1"* ]]
  
  run bash -c "echo 'echo Client 2' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"Client 2"* ]]
  
  stop_server "$TEST_PORT"
}

@test "Server: Client exits without stopping server" {
  TEST_PORT=5557
  start_server "$TEST_PORT" false
  
  run bash -c "echo 'exit' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  sleep 1
  run pgrep -f "./dsh -s -p $TEST_PORT"
  [ "$status" -eq 0 ]
  
  stop_server "$TEST_PORT"
}

@test "Remote: Simple command execution" {
  TEST_PORT=5558
  start_server "$TEST_PORT" false
  
  run bash -c "echo 'echo Hello from remote shell' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"Hello from remote shell"* ]]
  
  stop_server "$TEST_PORT"
}

@test "Remote: Command with pipe" {
  TEST_PORT=5559
  start_server "$TEST_PORT" false
  run bash -c "echo 'echo dsh test | grep dsh' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"dsh"* ]]
  stop_server "$TEST_PORT"
}

@test "Remote: Multiple commands" {
  TEST_PORT=5560
  start_server "$TEST_PORT" false
  
  run bash -c "echo -e 'echo Command 1\necho Command 2\nexit' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"Command 1"* ]]
  [[ "$trimmed_output" == *"Command 2"* ]]
  
  stop_server "$TEST_PORT"
}

@test "Remote: Directory change persists across commands" {
  TEST_PORT=5561
  start_server "$TEST_PORT" false
  
  run bash -c "echo -e 'cd /tmp\npwd\nexit' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"/tmp"* ]]
  
  stop_server "$TEST_PORT"
}

# -----------------------------
# Error Handling Tests
# -----------------------------


@test "Remote: Handles large output" {
  TEST_PORT=5563
  start_server "$TEST_PORT" false
  run bash -c "echo 'seq 1 1000' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"1000"* ]]
  stop_server "$TEST_PORT"
}

@test "Remote: Background processes" {
  TEST_PORT=5564
  start_server "$TEST_PORT" false
  
  run bash -c "echo 'sleep 1 &' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  
  stop_server "$TEST_PORT"
}

@test "Remote: Complex pipelines" {
  TEST_PORT=5565
  start_server "$TEST_PORT" false
  
  run bash -c "echo 'ls -la | grep dsh | wc -l' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  
  stop_server "$TEST_PORT"
}

@test "Remote: Command with redirection" {
  TEST_PORT=5566
  start_server "$TEST_PORT" false
  
  local temp_file="/tmp/dsh_test_file.txt"
  run bash -c "echo 'echo Test > $temp_file' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  
  run bash -c "echo 'cat $temp_file' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"Test"* ]]
  
  run bash -c "echo 'rm $temp_file' | ./dsh -c -p $TEST_PORT"
  
  stop_server "$TEST_PORT"
}

# -----------------------------
# EXTRA CREDIT: Multi-threaded Server Tests
# -----------------------------

@test "EXTRA CREDIT: Multi-threaded server starts with -x flag" {
  TEST_PORT=6000
  start_server "$TEST_PORT" true
  
  run pgrep -f "./dsh -s -p $TEST_PORT -x"
  [ "$status" -eq 0 ]
  
  stop_server "$TEST_PORT"
}

@test "EXTRA CREDIT: Multi-threaded server handles basic commands" {
  TEST_PORT=6001
  start_server "$TEST_PORT" true
  
  run bash -c "echo 'echo Hello from threaded server' | ./dsh -c -p $TEST_PORT"
  [ "$status" -eq 0 ]
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"Hello from threaded server"* ]]
  
  stop_server "$TEST_PORT"
}

@test "EXTRA CREDIT: Multi-threaded server handles concurrent connections" {
  TEST_PORT=6002
  start_server "$TEST_PORT" true
  # Send individual commands that dsh can parse, avoiding semicolons
  bash -c "printf 'sleep 10\necho Client 1 done\n' | ./dsh -c -i 127.0.0.1 -p $TEST_PORT > /tmp/client1.txt 2>&1" &
  pid1=$!
  sleep 1
  bash -c "echo 'echo Client 2 done' | ./dsh -c -i 127.0.0.1 -p $TEST_PORT > /tmp/client2.txt 2>&1" &
  pid2=$!
  wait $pid1 $pid2
  run cat /tmp/client1.txt
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"Client 1 done"* ]]
  run cat /tmp/client2.txt
  trimmed_output="$(filter_dsh_noise "$output")"
  [[ "$trimmed_output" == *"Client 2 done"* ]]
  stop_server "$TEST_PORT"
  rm -f /tmp/client1.txt /tmp/client2.txt
}



@test "EXTRA CREDIT: Multi-threaded server stop command" {
  TEST_PORT=6005
  start_server "$TEST_PORT" true
  bash -c "echo 'sleep 3' | ./dsh -c -p $TEST_PORT > /dev/null 2>&1" &
  sleep 1
  stop_server "$TEST_PORT"
  run pgrep -f "./dsh -s -p $TEST_PORT -x"
  [ "$status" -ne 0 ]
}


# -----------------------------
# End-to-end Workflow Test
# -----------------------------

@test "End-to-end: Full workflow test" {
  TEST_PORT=5569
  start_server "$TEST_PORT" false
  
  # Create a test file and run a series of commands.
  run bash -c "echo -e 'cd /tmp\necho \"Hello, world\" > dsh_e2e_test.txt\ncat dsh_e2e_test.txt\nls -la | grep dsh_e2e_test.txt\nrm dsh_e2e_test.txt\nexit' | ./dsh -c -p $TEST_PORT"
  
  [ "$status" -eq 0 ]
  
  # Debugging output
  echo "Original output:"
  echo "$output"
  
  filtered="$(filter_dsh_noise "$output")"
  
  echo "Filtered output:"
  echo "$filtered"
  
  [[ "$filtered" == *"Hello, world"* ]]
  [[ "$filtered" == *"dsh_e2e_test.txt"* ]]
  
  stop_server "$TEST_PORT"
}
