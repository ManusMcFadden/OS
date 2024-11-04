#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define NULL 0

/* Read a line of characters from stdin. */
int getcmd(char *buf, int nbuf) {
    // Print >>> then clear buffer
    printf(">>> ");
    memset(buf, 0, nbuf);

    // Read the user's input into buf
    gets(buf, nbuf);
    if (buf[0] == 0) {
        return -1;  // Return -1 if input is empty
    }

    // Remove the newline character if present
    for (int i = 0; i < nbuf; i++) {
        if (buf[i] == '\n') {
            buf[i] = '\0';  // Replace \n with null terminator
            break;  // Exit the loop after replacing
        }
    }
    return 0;  // Return 0 on success
}


/*
  A recursive function which parses the command
  at *buf and executes it.
*/
void run_command(char *buf, int nbuf, int *pcp) {
    
    char *arguments[10];
    int numargs = 0;
    int pipe_cmd = 0;
    int sequential = 0;
    int redirection_left = 0;
    int redirection_right = 0;
    char *file_name_l = NULL;
    char *file_name_r = NULL;
    char *second_command = NULL;
    int ws = 0;

    for (int i = 0; i < nbuf; i++) {
        if (buf[i] == '|' || buf[i] == ';') {
            if (buf[i] == '|') {
                pipe_cmd = 1;
            } else {
                sequential = 1;
            }
            buf[i] = '\0';  // End the first command
            i++;  // Skip delimiter
            while (buf[i] == ' ') i++;  // Skip spaces

            // Store the remaining command
            int second_command_length = strlen(&buf[i]) + 1;
            second_command = (char *)malloc(second_command_length);
            if (second_command == NULL) {
                fprintf(2,"Memory allocation failed\n");
                exit(1);
            }
            strcpy(second_command, &buf[i]);
            break;
        }

        if (buf[i] == '>') {
            buf[i] = '\0';  // End current argument


            redirection_right = 1;
            
            i++;
            while (buf[i] == ' ') i++;  // Skip spaces after `>`
            file_name_r = &buf[i];
            continue;
        }

        if (buf[i] == '<') {
            buf[i] = '\0';  // End current argument
            redirection_left = 1;
            i++;
            while (buf[i] == ' ') i++;  // Skip spaces after `<`
            file_name_l = &buf[i];
            continue;
        }

        if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\0') {
            if (ws != i) {
                buf[i] = '\0';
                if (numargs < 10) {
                    arguments[numargs++] = &buf[ws];
                }
            }
            ws = i + 1;
        }
    }

    if (ws < nbuf && buf[ws] != '\0' && numargs < 10) {
        arguments[numargs++] = &buf[ws];
    }
    arguments[numargs] = NULL;

    
    if (pipe_cmd && second_command != NULL) {
        // create a new pipe to pass data inbetween
        int pcp[2];
        // create the pipe
        if (pipe(pcp) == -1){
            fprintf(2,"Pipe failed\n");
            exit(1);
        }
        // fork for the left command
        if (fork() == 0){
            // in the left child process
            // close ....
            close(pcp[0]);
            close(1);
            if (dup(pcp[1]) == -1){
                fprintf(2,"Dup failed in the left command\n");
                exit(1);
            }
            close(pcp[1]);

            // now execute the left command
            if (exec(arguments[0], arguments) == -1){
                fprintf(2,"Exec failed for left command\n");
                exit(1);
            }
        } 

        // fork now for the right command reading from pipe
        if (fork() == 0){
            close(pcp[1]);
            close(0);
            if (dup(pcp[0]) == -1){
                fprintf(2, "Dup failed in right command\n");
                exit(1);
            }
            close(pcp[0]);
            run_command(second_command, strlen(second_command), pcp);
            free(second_command);
            exit(0);
        }
        // close both ends of pipe
        close(pcp[0]);
        close(pcp[1]);
        // wait for child processes to finish
        wait(0);
        wait(0);

    } else {
        // If not a pipe command
        int pid = fork();  // Fork a new process
        if (pid == 0) {
            // check for redirection commands
            if (redirection_right){
                int fd = open(file_name_r, O_WRONLY | O_CREATE | O_TRUNC);
                if (fd < 0){
                fprintf(2,"Failed to open %s for writing\n", file_name_r);
                exit(1);
                }
                // close the standard output, then duplicate output to file
                close(1);
                dup(fd);
                close(fd);
            } else if (redirection_left){
                // Open the file for reading
                int fd = open(file_name_l, O_RDONLY);
                if (fd < 0) {
                    fprintf(2,"Failed to open %s for reading\n", file_name_l);
                    exit(1);
                }
                
                // Close stdin and redirect it to the file
                close(0); // Close the original stdin
                dup(fd); // Duplicate fd onto stdin (0)
                close(fd); // Close the file descriptor as it's no longer needed
                // need to work out how many args before '<'
                
                if (numargs > 1){
                    arguments[numargs - 1] = 0;
                }
                
            } 
            // Child process
            exec(arguments[0], arguments);
            printf("exec failed\n");  // Exec failed
            exit(1);
            
            
        } else if (pid > 0) {
            // Parent process waits for the child to complete
            wait(NULL); 

            // Handle sequential commands
            if (sequential && second_command != NULL) {
                //printf("Running second command from ; : %s\n", second_command);
                run_command(second_command, strlen(second_command), pcp);
                free(second_command);
            }
        } else {
            printf("fork failed\n");
        }
    } 
}


int main(void) {
    static char buf[100];
    int pcp[2];
    pipe(pcp);
    // Read and run input commands
    // this main function has been made just to ensure cd works, as it needs to be executed outside fork
    while (getcmd(buf, sizeof(buf)) >= 0) {
        char *arguments[10];  // Array to hold command arguments
        int numargs = 0;
        int ws = 0;

        // Loop through the input buffer to extract arguments and run
        for (int i = 0; i < strlen(buf); i++) {
            if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\0') {
                if (ws != i) {  // Ensure we aren't capturing empty arguments
                    buf[i] = '\0';  // Null-terminate the current argument
                    if (numargs < 10) {
                        
                        arguments[numargs] = &buf[i + 1];  // Add the argument to the list
                        numargs++;
                    }
                }
                ws = i + 1;  // Update the word start to the next character
            }
        }
        // Make sure we properly null-terminate the argument list
        arguments[numargs] = 0;

        // Handle the "cd" command in the parent process
        if (numargs > 0 && strcmp(buf, "cd") == 0) {
            if (numargs < 1) {
                printf("cd: missing argument\n");
            } else if (chdir(arguments[0]) != 0) {
                printf("cd: %s: No such directory\n", arguments[0]);
            }
            continue;  // Skip the rest of the loop to avoid forking for "cd"
        }
        
        // For all other commands, fork a new process
        if (fork() == 0) {
            // In the child process, execute the command
            run_command(buf, 100, pcp);
        }

        int child_status;
        wait(&child_status);  // Wait for the child process to complete
    }
    exit(0);
}