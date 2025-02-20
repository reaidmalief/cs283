#!/usr/bin/env bats

# File: student_tests.sh
# 

@test "Basic external command execution" {
    run ./dsh <<EOF
ls -l
EOF
    [ "$status" -eq 0 ]
}

@test "Change directory command" {
    run ./dsh <<EOF
cd /tmp
pwd
EOF
    [[ "$output" =~ "/tmp" ]]
    [ "$status" -eq 0 ]
}

@test "Change directory with no args" {
    current=$(pwd)
    run ./dsh <<EOF
cd
pwd
EOF
    [[ "$output" =~ "$current" ]]
    [ "$status" -eq 0 ]
}

@test "Handle quoted strings" {
    run ./dsh <<EOF
echo "hello   world"
EOF
    [[ "$output" =~ "hello   world" ]]
    [ "$status" -eq 0 ]
}

@test "Handle empty input" {
    run ./dsh <<EOF

EOF
    [ "$status" -eq 0 ]
}

@test "Handle multiple spaces between arguments" {
    run ./dsh <<EOF
echo    hello     world
EOF
    [[ "$output" =~ "hello world" ]]
    [ "$status" -eq 0 ]
}

@test "Exit command" {
    run ./dsh <<EOF
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Execute command from PATH" {
    run ./dsh <<EOF
which ls
EOF
    [[ "$output" =~ "/bin/ls" ]] || [[ "$output" =~ "/usr/bin/ls" ]]
    [ "$status" -eq 0 ]
}

# Return code handling tests
@test "Command not found returns appropriate error" {
    run ./dsh <<EOF
nonexistentcommand
rc
EOF
    [[ "$output" =~ "Command not found in PATH" ]]
    [[ "$output" =~ "2" ]]  # ENOENT usually maps to 2
}

@test "Permission denied handling" {
    run ./dsh <<EOF
touch testfile
chmod 000 testfile
./testfile
rc
rm -f testfile
EOF
    [[ "$output" =~ "Permission denied" ]]
    [[ "$output" =~ "13" ]]  # EACCES usually maps to 13
}

@test "Return code shows success after successful command" {
    run ./dsh <<EOF
true
rc
EOF
    [[ "$output" =~ "0" ]]
}

@test "Return code shows failure after failed command" {
    run ./dsh <<EOF
false
rc
EOF
    [[ "$output" =~ "1" ]]
}

@test "Return code persists between commands" {
    run ./dsh <<EOF
false
rc
echo hello
rc
EOF
    [[ "$output" =~ "hello" ]]
    [[ "$output" =~ "1" ]]
}

@test "prints Dragon ASCII art" {
    run ./dsh <<EOF
dragon
exit
EOF

    [[ "$output" =~ "                                                   @%%%%                       
                                                                     %%%%%%                         
                                                                    %%%%%%                          
                                                                 % %%%%%%%           @              
                                                                %%%%%%%%%%        %%%%%%%           
                                       %%%%%%%  %%%%@         %%%%%%%%%%%%@    %%%%%%  @%%%%        
                                  %%%%%%%%%%%%%%%%%%%%%%      %%%%%%%%%%%%%%%%%%%%%%%%%%%%          
                                %%%%%%%%%%%%%%%%%%%%%%%%%%   %%%%%%%%%%%% %%%%%%%%%%%%%%%           
                               %%%%%%%%%%%%%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%     %%%            
                             %%%%%%%%%%%%%%%%%%%%%%%%%%%%@ @%%%%%%%%%%%%%%%%%%        %%            
                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%%%%                
                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              
                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%@%%%%%%@              
      %%%%%%%%@           %%%%%%%%%%%%%%%%        %%%%%%%%%%%%%%%%%%%%%%%%%%      %%                
    %%%%%%%%%%%%%         %%@%%%%%%%%%%%%           %%%%%%%%%%% %%%%%%%%%%%%      @%                
  %%%%%%%%%%   %%%        %%%%%%%%%%%%%%            %%%%%%%%%%%%%%%%%%%%%%%%                        
 %%%%%%%%%       %         %%%%%%%%%%%%%             %%%%%%%%%%%%@%%%%%%%%%%%                       
%%%%%%%%%@                % %%%%%%%%%%%%%            @%%%%%%%%%%%%%%%%%%%%%%%%%                     
%%%%%%%%@                 %%@%%%%%%%%%%%%            @%%%%%%%%%%%%%%%%%%%%%%%%%%%%                  
%%%%%%%@                   %%%%%%%%%%%%%%%           %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              
%%%%%%%%%%                  %%%%%%%%%%%%%%%          %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%      %%%%  
%%%%%%%%%@                   @%%%%%%%%%%%%%%         %%%%%%%%%%%%@ %%%% %%%%%%%%%%%%%%%%%   %%%%%%%%
%%%%%%%%%%                  %%%%%%%%%%%%%%%%%        %%%%%%%%%%%%%      %%%%%%%%%%%%%%%%%% %%%%%%%%%
%%%%%%%%%@%%@                %%%%%%%%%%%%%%%%@       %%%%%%%%%%%%%%     %%%%%%%%%%%%%%%%%%%%%%%%  %%
 %%%%%%%%%%                  % %%%%%%%%%%%%%%@        %%%%%%%%%%%%%%   %%%%%%%%%%%%%%%%%%%%%%%%%% %%
  %%%%%%%%%%%%  @           %%%%%%%%%%%%%%%%%%        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  %%% 
   %%%%%%%%%%%%% %%  %  %@ %%%%%%%%%%%%%%%%%%          %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    %%% 
    %%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%%%%           @%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    %%%%%%% 
     %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              %%%%%%%%%%%%%%%%%%%%%%%%%%%%        %%%   
      @%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                  %%%%%%%%%%%%%%%%%%%%%%%%%               
        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                      %%%%%%%%%%%%%%%%%%%  %%%%%%%          
           %%%%%%%%%%%%%%%%%%%%%%%%%%                           %%%%%%%%%%%%%%%  @%%%%%%%%%         
              %%%%%%%%%%%%%%%%%%%%           @%@%                  @%%%%%%%%%%%%%%%%%%   %%%        
                  %%%%%%%%%%%%%%%        %%%%%%%%%%                    %%%%%%%%%%%%%%%    %         
                %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                      %%%%%%%%%%%%%%            
                %%%%%%%%%%%%%%%%%%%%%%%%%%  %%%% %%%                      %%%%%%%%%%  %%%@          
                     %%%%%%%%%%%%%%%%%%% %%%%%% %%                          %%%%%%%%%%%%%@          
                                                                                 %%%%%%%@" ]]
    [ "$status" -eq 0 ]
}