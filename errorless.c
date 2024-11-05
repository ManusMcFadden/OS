#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

/* Read a line of characters from stdin. */
int getcmd(char *buf, int nbuf) {
  printf(">>> ");
  memset(buf, 0, nbuf);
  if (gets(buf, nbuf) == 0) {
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

void run_command(char *buf, int nbuf, int *pcp) {
  char *arguments[10];
  int numargs = 0;
  int ws = 1;

  int redirection_left = 0;
  int redirection_right = 0;
  char *file_name_l = 0;
  char *file_name_r = 0;

  int pipe_cmd = 0;
  int sequence_cmd = 0;

  int i = 0;
  for (; i < nbuf; i++) {
    // Stop processing arguments if we encounter a redirection or special character
    if (buf[i] == '<') {
      redirection_left = 1;
      buf[i] = '\0';
      file_name_l = &buf[i + 1];
      while (*file_name_l == ' ') file_name_l++;  // Trim spaces after '<'
      break; // Stop adding to arguments
    }
    if (buf[i] == '>') {
      redirection_right = 1;
      buf[i] = '\0';
      file_name_r = &buf[i + 1];
      while (*file_name_r == ' ') file_name_r++;  // Trim spaces after '>'
      break; // Stop adding to arguments
    }
    if (buf[i] == '|') {
      pipe_cmd = 1;
      buf[i] = '\0';
      break; // Stop adding to arguments
    }
    if (buf[i] == ';') {
      sequence_cmd = 1;
      buf[i] = '\0';
      break; // Stop adding to arguments
    }

    if (buf[i] != ' ' && buf[i] != '\0' && buf[i] != '\n' && ws) {
      arguments[numargs++] = &buf[i];
      ws = 0;
    } else if (buf[i] == ' ' || buf[i] == '\0' || buf[i] == '\n') {
      buf[i] = '\0';
      ws = 1;
    }

    if (numargs >= 10) break;
  }

  if (numargs == 0) exit(1);
  arguments[numargs] = 0;

  if (pipe_cmd) {
    // Handle pipe command
    int pipe_fd[2];
    if (pipe(pipe_fd) < 0) {
      printf("Error: Unable to create pipe\n");
      exit(1);
    }

    if (fork() == 0) {
      // First part of the pipeline (left command)
      close(pipe_fd[0]);             // Close the read end of the pipe
      close(1);                       // Close stdout
      dup(pipe_fd[1]);                // Redirect stdout to pipe's write end
      close(pipe_fd[1]);

      exec(arguments[0], arguments);  // Execute the first command
      printf("Error: exec failed for %s\n", arguments[0]);
      exit(1);
    }

    if (fork() == 0) {
      // Second part of the pipeline (right command)
      close(pipe_fd[1]);              // Close the write end of the pipe
      close(0);                       // Close stdin
      dup(pipe_fd[0]);                // Redirect stdin to pipe's read end
      close(pipe_fd[0]);

      char *right_command[10];
      int numargs_right = 0;
      char *ptr = &buf[i + 1];
      ws = 1;

      // Parse the second part of the command (right side of the pipe)
      for (; *ptr != '\0' && numargs_right < 10; ptr++) {
        if (*ptr != ' ' && ws) {
          right_command[numargs_right++] = ptr;
          ws = 0;
        } else if (*ptr == ' ') {
          *ptr = '\0';
          ws = 1;
        }
      }
      right_command[numargs_right] = 0;

      exec(right_command[0], right_command);  // Execute the second command
      printf("Error: exec failed for %s\n", right_command[0]);
      exit(1);
    }

    // Parent process closes both ends of the pipe and waits for children
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    wait(0);
    wait(0);

  } else {
    if (sequence_cmd) {
      sequence_cmd = 0;
      if (fork() != 0) {
        wait(0);
      }
    }

    if (redirection_left) {
      int fd = open(file_name_l, O_RDONLY);
      if (fd < 0) {
        printf("Error: Unable to open file for reading: %s\n", file_name_l);
        exit(1);
      }
      close(0);
      dup(fd);
      close(fd);
    }

    if (redirection_right) {
      if (file_name_r && *file_name_r != '\0') {
        int fd = open(file_name_r, O_WRONLY | O_CREATE | O_TRUNC);
        if (fd < 0) {
          printf("Error: Unable to open file for writing: %s\n", file_name_r);
          exit(1);
        }
        close(1);
        dup(fd);
        close(fd);
      } else {
        printf("Error: No file name specified after '>'\n");
        exit(1);
      }
    }

    if (strcmp(arguments[0], "cd") == 0) {
      close(pcp[0]);
      write(pcp[1], arguments[1], strlen(arguments[1]) + 1);
      close(pcp[1]);
      exit(2);
    } else {
      if (fork() == 0) {
        exec(arguments[0], arguments);
        printf("Error: exec failed for %s\n", arguments[0]);
        exit(1);
      } else {
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

  while (getcmd(buf, sizeof(buf)) >= 0) {
    if (fork() == 0) {
      run_command(buf, 100, pcp);
    } else {
      if (wait(0) == 2) {
        char temp[100];
        close(pcp[1]);
        read(pcp[0], temp, sizeof(temp));
        close(pcp[0]);
        chdir(temp);
      }
    }
  }
  exit(0);
}
