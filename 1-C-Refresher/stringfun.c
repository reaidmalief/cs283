#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);

//prototypes for functions to handle required functionality
int count_words(char *, int, int);
//add additional prototypes here
int reverse_string(char *, int, int);
int print_words(char *, int, int);
int replace_string(char *buff, int len, int str_len, char *search, char *replace);

int setup_buff(char *buff, char *user_str, int len) {
    //TODO: #4:  Implement the setup buff as per the directions
    if (!buff || !user_str || len <= 0){
        return -2;
    } 
    
    int buff_pos = 0;
    char *input_ptr = user_str;
    int prev_space = 1;  // Start assuming previous char was space to handle leading spaces
    
    // Process input string
    while (*input_ptr != '\0') {
        if (*input_ptr == ' ' || *input_ptr == '\t') {
            if (!prev_space && buff_pos < len) {
                *(buff + buff_pos) = ' ';
                buff_pos++;
            }
            prev_space = 1;
        } else {
            if (buff_pos >= len) {
                return -1;  // String too long
            }
            *(buff + buff_pos) = *input_ptr;
            buff_pos++;
            prev_space = 0;
        }
        input_ptr++;
    }
    
    // Remove trailing space if exists
    if (buff_pos > 0 && *(buff + buff_pos - 1) == ' ') {
        buff_pos--;
    }
    
    // Fill remainder with dots
    while (buff_pos < len) {
        *(buff + buff_pos) = '.';
        buff_pos++;
    }
    
    return buff_pos;
}

void print_buff(char *buff, int len){
    printf("Buffer:  [");
    for (int i=0; i<len; i++){
        putchar(*(buff+i));
    }
    printf("]\n");
}

void usage(char *exename){
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}


int count_words(char *buff, int len, int str_len){
    if (!buff || len <= 0 || str_len > len) return -1;
    
    int word_count = 0;
    int in_word = 0;
    char *ptr = buff;
    
    while (ptr < buff + str_len && *ptr != '.') {
        if (*ptr != ' ' && !in_word) {
            word_count++;
            in_word = 1;
        } else if (*ptr == ' ') {
            in_word = 0;
        }
        ptr++;
    }
    
    return word_count;
}

int reverse_string(char *buff, int len, int str_len) {
    if (!buff || len <= 0 || str_len > len) return -1;
    
    // Find actual content length (until first dot)
    int actual_len = 0;
    char *ptr = buff;
    while (ptr < buff + str_len && *ptr != '.') {
        actual_len++;
        ptr++;
    }
    
    // Reverse only the content (not the dots)
    char *start = buff;
    char *end = buff + actual_len - 1;
    
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return 0;
}

int print_words(char *buff, int len, int str_len) {
    if (!buff || len <= 0 || str_len > len) return -1;
    
    printf("Word Print\n----------\n");
    
    int word_count = 0;
    int char_count = 0;
    int in_word = 0;
    char *word_start = buff;
    char *ptr = buff;
    
    while (ptr < buff + str_len && *ptr != '.') {
        if (*ptr != ' ' && !in_word) {
            word_start = ptr;
            in_word = 1;
            char_count = 1;
        } else if (*ptr != ' ' && in_word) {
            char_count++;
        } else if (*ptr == ' ' && in_word) {
            word_count++;
            printf("%d. ", word_count);
            char *temp = word_start;
            while (temp < ptr) {
                putchar(*temp);
                temp++;
            }
            printf("(%d)\n", char_count);
            in_word = 0;
        }
        ptr++;
    }
    
    // Handle last word if exists
    if (in_word) {
        word_count++;
        printf("%d. ", word_count);
        char *temp = word_start;
        while (temp < ptr) {
            putchar(*temp);
            temp++;
        }
        printf("(%d)\n", char_count);
    }
    
    return word_count;
}

int replace_string(char *buff, int len, int str_len, char *search, char *replace) {
    if (!buff || !search || !replace || len <= 0 || str_len > len) return -1;
    
    // Get lengths of search and replace strings
    int search_len = 0;
    int replace_len = 0;
    char *ptr = search;
    while (*ptr != '\0') {
        search_len++;
        ptr++;
    }
    
    ptr = replace;
    while (*ptr != '\0') {
        replace_len++;
        ptr++;
    }
    
    // Find search string in buffer
    char *start = buff;
    char *end = buff + str_len;
    char *found = NULL;
    
    while (start < end && *start != '.') {
        // Check if search string matches at current position
        int matches = 1;
        for (int i = 0; i < search_len; i++) {
            if (start + i >= end || *(start + i) == '.' || *(start + i) != search[i]) {
                matches = 0;
                break;
            }
        }
        
        if (matches) {
            found = start;
            break;
        }
        start++;
    }
    
    // If search string not found
    if (!found) {
        return -2;  // Pattern not found
    }
    
    // Calculate new string length after replacement
    int new_len = str_len - search_len + replace_len;
    if (new_len > len) {
        // Handle overflow by truncating
        new_len = len;
    }
    
    // Make space for replacement if needed
    if (replace_len != search_len) {
        char *src = found + search_len;
        char *dst = found + replace_len;
        int move_len = end - (found + search_len);
        
        // If replacement is shorter, move left
        if (replace_len < search_len) {
            while (move_len > 0 && *src != '.') {
                *dst = *src;
                src++;
                dst++;
                move_len--;
            }
        }
        // If replacement is longer, move right (but don't exceed buffer)
        else {
            // Calculate how many characters we can safely move
            int max_move = len - (found - buff) - replace_len;
            if (max_move > move_len) max_move = move_len;
            
            src = found + search_len + max_move - 1;
            dst = found + replace_len + max_move - 1;
            
            while (move_len > 0 && max_move > 0) {
                *dst = *src;
                src--;
                dst--;
                move_len--;
                max_move--;
            }
        }
    }
    
    // Copy in replacement string
    for (int i = 0; i < replace_len && (found + i) < (buff + len); i++) {
        *(found + i) = replace[i];
    }
    
    // Fill rest with dots if needed
    ptr = buff;
    while (ptr < buff + len) {
        if (*ptr == '\0' || ptr >= buff + new_len) {
            *ptr = '.';
        }
        ptr++;
    }
    
    return new_len;
}

int main(int argc, char *argv[]){
    char *buff;             //placehoder for the internal buffer
    char *input_string;     //holds the string provided by the user on cmd line
    char opt;               //used to capture user option from cmd line
    int  rc;                //used for return codes
    int  user_str_len;      //length of user supplied string

    //TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
    /*      This check is safe because the if condition first checks if (argc < 2) before attempting to access
            argv[1]. If there aren't enough arguments, the program will exit before trying to access argv[1], 
            preventing any buffer overflow or segmentation errors. The second part (*argv[1] != '-') is only 
            evaluated if the first condition is false, insuring argv[1] exist 
    */
    if ((argc < 2) || (*argv[1] != '-')){
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1]+1);   //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h'){
        usage(argv[0]);
        exit(0);
    }

    //WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    //TODO:  #2 Document the purpose of the if statement below
    //      PLACE A COMMENT BLOCK HERE EXPLAINING
    /*
        This if statement checks if the number of arguments provided is less than 3.
        If there are fewer than 3 arguments, it means the user did not provide the required string input.
        In such a case, the usage function is called to display the correct usage of the program,
        and the program exits with a status code of 1, indicating an error due to insufficient arguments.
    */
    if (argc < 3){
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; //capture the user input string

    //TODO:  #3 Allocate space for the buffer using malloc and
    //          handle error if malloc fails by exiting with a 
    //          return code of 99
    buff = (char *)malloc(BUFFER_SZ);
    if (!buff) {
        printf("Failed to allocate memory\n");
        exit(99);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);     //see todos
    if (user_str_len < 0){
        printf("Error setting up buffer, error = %d\n", user_str_len);
        free(buff);
        exit(2);
    }

    switch (opt){
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);  //you need to implement
            if (rc < 0){
                printf("Error counting words, rc = %d\n", rc);
                free(buff);
                exit(2);
            }
            printf("Word Count: %d\n", rc);
            break;

        //TODO:  #5 Implement the other cases for 'r' and 'w' by extending
        //       the case statement options
        case 'r':
            rc = reverse_string(buff, BUFFER_SZ, user_str_len);
            if (rc < 0) {
                printf("Error reversing string, rc = %d\n", rc);
                free(buff);
                exit(2);
            }
            break;
            
        case 'w':
            rc = print_words(buff, BUFFER_SZ, user_str_len);
            if (rc < 0) {
                printf("Error printing words, rc = %d\n", rc);
                free(buff);
                exit(2);
            }
            printf("\nNumber of words returned: %d\n", rc);
            break;
            
        case 'x':
            if (argc < 5) {
                printf("Error: -x option requires two additional arguments\n");
                free(buff);
                exit(1);
            }
            rc = replace_string(buff, BUFFER_SZ, user_str_len, argv[3], argv[4]);
            if (rc == -2) {
                printf("Error: Search string not found\n");
                free(buff);
                exit(3);
            }
            if (rc < 0) {
                printf("Error performing string replacement, rc = %d\n", rc);
                free(buff);
                exit(2);
            }
            user_str_len = rc;  // Update string length after replacement
            break;
            
        default:
            usage(argv[0]);
            free(buff);
            exit(1);
    }

    //TODO:  #6 Dont forget to free your buffer before exiting
    print_buff(buff, BUFFER_SZ);
    free(buff);
    exit(0);
}

//TODO:  #7  Notice all of the helper functions provided in the 
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that 
//          the buff variable will have exactly 50 bytes?
//  
/*      Passing both the pointer and length is a good practice for several reasons. 
        Firstly, it enhances safety by preventing buffer overflows, ensuring that functions do not read or write beyond the allocated memory. 
        Secondly, it provides flexibility, allowing functions to handle buffers of varying sizes rather than being restricted to a fixed size like BUFFER_SZ. This makes the functions more versatile. 
        Thirdly, it promotes modularity, making the functions easier to maintain since they do not rely on global constants. 
        Additionally, it improves robustness; even if BUFFER_SZ changes, the functions will still work correctly, reducing the risk of bugs. 
        Lastly, it aids maintainability by simplifying debugging and validation, as it is clear how much data the function should process. 
*/