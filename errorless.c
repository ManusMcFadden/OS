#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

/* Read a line of characters from stdin. */
int getcmd(char *buf, int nbuf) {

  printf(">>> ");
  memset(buf, 0, nbuf);
  if(gets(buf, nbuf) == 0) {
      return -1;
  }
  for (int i = 0; i < nbuf; i++) {
      if (buf[i] == '\n') {
        buf[i] = '\0';
        break;
      }
  }
  return 0;
}

/*
  A recursive function which parses the command
  at *buf and executes it.
*/
// __attribute__((noreturn))
void run_command(char *buf, int nbuf, int *pcp) {

  /* Useful data structures and flags. */
  char *arguments[10];
  int numargs = 0;
  /* Word start/end */
  int ws = 1;
  // int we = 0;  // Unused variable

  int redirection_left = 0;
  int redirection_right = 0;
  // char *file_name_l = 0;  // Unused variable
  // char *file_name_r = 0;  // Unused variable

  // int p[2];  // Unused variable
  int pipe_cmd = 0;
  int sequence_cmd = 0;

  int i = 0;
  /* Parse the command character by character. */
  for (; i < nbuf; i++) {

    /* Parse the current character and set-up various flags:
       sequence_cmd, redirection, pipe_cmd and similar. */

    if (buf[i] != ' ' && buf[i] != '\0' && buf[i] != '\n' && ws) {
      arguments[numargs++] = &buf[i];
      ws = 0;
    }
    else if (buf[i] == ' ' || buf[i] == '\0' || buf[i] == '\n') {
      buf[i] = '\0';
      ws = 1;
    }
    if (numargs >= 10) break;

    if(buf[i] == '<') {
      redirection_left = 1;
    }

    if(buf[i] == '>') {
      redirection_right = 1;
    }

    if(buf[i] == '|') {
      pipe_cmd = 1;
    }

    if(buf[i] == ';') {
      sequence_cmd = 1;
    }
  }

  if (numargs == 0) exit;

  arguments[numargs] = 0;

  /*
    Sequence command. Continue this command in a new process.
    Wait for it to complete and execute the command following ';'.
  */
  if (sequence_cmd) {
    sequence_cmd = 0;
    if (fork() != 0) {
      wait(0);
    }
  }

  /*
    If this is a redirection command,
    tie the specified files to std in/out.
  */
  if (redirection_left) {
    // Handle left redirection
  }
  if (redirection_right) {
    // Handle right redirection
  }

  /* Parsing done. Execute the command. */

  /*
    If this command is a CD command, write the arguments to the pcp pipe
    and exit with '2' to tell the parent process about this.
  */
  if (strcmp(arguments[0], "cd") == 0) {
    close(pcp[0]);
    write(pcp[1], arguments[1], strlen(arguments[1]) + 1);
    close(pcp[1]);
    exit(2);
  } else {
    /*
      Pipe command: fork twice. Execute the left hand side directly.
      Call run_command recursion for the right side of the pipe.
    */
    if (pipe_cmd) {
      // Handle pipe command
    }
    else {
      if (fork() == 0) {
        exec(arguments[0], arguments);
        exit(1);
      }
      else {
        wait(0);
      }
    }
  }
  exit(0);
}


int main(void) {

  static char buf[100];
  int pcp[2];
  pipe(pcp);

  /* Read and run input commands. */
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(fork() == 0) {
      run_command(buf, 100, pcp);
    /*
      Check if run_command found this is
      a CD command and run it if required.
    */
    }
    else {
      int child_status;
      if (wait(child_status) == 2) {
      char temp[100];
      close(pcp[1]);
      read(pcp[0], temp, sizeof(temp));
      close(pcp[0]);
      chdir(temp);
      }
    }
    exit(0);
  }
}
