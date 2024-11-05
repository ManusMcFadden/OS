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
    if (buf[i] != ' ' && buf[i] != '\0' && buf[i] != '\n' && ws) {
      arguments[numargs++] = &buf[i];
      ws = 0;
    }
    else if (buf[i] == ' ' || buf[i] == '\0' || buf[i] == '\n') {
      buf[i] = '\0';
      ws = 1;
    }
    if (numargs >= 10) break;

    if (buf[i] == '<') {
      redirection_left = 1;
      buf[i] = '\0';
      file_name_l = &buf[i + 1];
      // Trim spaces after '<'
      while (*file_name_l == ' ') file_name_l++;
    }

    if (buf[i] == '>') {
      redirection_right = 1;
      buf[i] = '\0';
      file_name_r = &buf[i + 1];
      // Trim spaces after '>'
      while (*file_name_r == ' ') file_name_r++;
      printf("Debug: File name for output redirection detected as '%s'\n", file_name_r);  // Debugging
    }

    if (buf[i] == '|') {
      pipe_cmd = 1;
    }

    if (buf[i] == ';') {
      sequence_cmd = 1;
    }
  }

  if (numargs == 0) exit(1);
  arguments[numargs] = 0;

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
      printf("Attempting to open file for writing: '%s'\n", file_name_r);  // Debug output
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
    if (pipe_cmd) {
      // Handle pipe command
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
