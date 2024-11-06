#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int getcmd(char *buf, int nbuf) {
  printf(">>> ");
  memset(buf, 0, nbuf);
  if (gets(buf, nbuf) == 0) {
      fprintf(2, "Error: Couldnt get input\n");
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
  int p[2];

  int i = 0;
  for (; i < nbuf; i++) {
    if (buf[i] == '<') {
      redirection_left = 1;
      buf[i] = '\0';
      file_name_l = &buf[i + 1];
      while (*file_name_l == ' ') file_name_l++;
      if (*file_name_l == '\0') {
        fprintf(2, "Error: Need a filename before <\n");
        return;
      }
      break;
    }
    if (buf[i] == '>') {
      redirection_right = 1;
      buf[i] = '\0';
      file_name_r = &buf[i + 1];
      while (*file_name_r == ' ') file_name_r++;
      if (*file_name_r == '\0') {
        fprintf(2, "Error: Need a file name after >\n");
        return;
      }
      break;
    }
    if (buf[i] == '|') {
      pipe_cmd = 1;
      buf[i] = '\0';
      break;
    }
    if (buf[i] == ';') {
      sequence_cmd = 1;
      buf[i] = '\0';
      break;
    }
    if (buf[i] != ' ' && buf[i] != '\0' && buf[i] != '\n' && ws) {
      arguments[numargs++] = &buf[i];
      ws = 0;
    } else if (buf[i] == ' ' || buf[i] == '\0' || buf[i] == '\n') {
      buf[i] = '\0';
      ws = 1;
    }
    if (numargs >= 10) {
      fprintf(2, "Error: Cannot have more than 10 arguments\n");
      return;
    }
  }

  if (numargs == 0) {
    fprintf(2, "Error: Nothing typed\n");
    return;
  }

  arguments[numargs] = 0;

  if (pipe_cmd) {
    if (pipe(p) < 0) {
      fprintf(2, "Error: Cannot make pipe\n");
      return;
    }
    if (fork() == 0) {
      close(p[0]);
      if (dup(p[1]) < 0) {
        fprintf(2, "Error: dup error\n");
        exit(1);
      }
      close(p[1]);
      exec(arguments[0], arguments);
      fprintf(2, "Error: Cannot execute %s\n", arguments[0]);
      exit(1);
    }
    close(p[1]);
    if (fork() == 0) {
      close(0);
      if (dup(p[0]) < 0) {
        fprintf(2, "Error: dup error\n");
        exit(1);
      }
      close(p[0]);
      char *rightCmd = &buf[i + 1];
      run_command(rightCmd, nbuf - (i + 1), pcp);
      exit(0);
    }
    close(p[0]);
    wait(0);
    wait(0);
    return;
  }

  if (sequence_cmd) {
    if (fork() == 0) {
      run_command(buf, i, pcp);
      exit(0);
    }
    wait(0);
    char *nextCmd = &buf[i + 1];
    run_command(nextCmd, nbuf - (i + 1), pcp);
    return;
  }

  if (redirection_left) {
    int fd = open(file_name_l, O_RDONLY);
    if (fd < 0) {
      fprintf(2, "Error: Cannot open file for reading: %s\n", file_name_l);
      return;
    }
    close(0);
    if (dup(fd) < 0) {
      fprintf(2, "Error: dup error\n");
      close(fd);
      return;
    }
    close(fd);
  }

  if (redirection_right) {
    int fd = open(file_name_r, O_WRONLY | O_CREATE | O_TRUNC);
    if (fd < 0) {
      fprintf(2, "Error: Cannot open file for writing: %s\n", file_name_r);
      return;
    }
    close(1);
    if (dup(fd) < 0) {
      fprintf(2, "Error: dup error\n");
      close(fd);
      return;
    }
    close(fd);
  }

  if (strcmp(arguments[0], "cd") == 0) {
    close(pcp[0]);
    if (arguments[1] == 0) {
      fprintf(2, "Error: cd but no directory after it\n");
      return;
    }
    write(pcp[1], arguments[1], strlen(arguments[1]) + 1);
    close(pcp[1]);
    exit(2);
  } else {
    if (fork() == 0) {
      exec(arguments[0], arguments);
      fprintf(2, "Error: Cannot execute %s\n", arguments[0]);
      exit(1);
    }
    wait(0);
  }
}

int main(void) {
  static char buf[100];
  int pcp[2];
  if (pipe(pcp) < 0) {
    fprintf(2, "Error: Cannot make pipe\n");
    exit(1);
  }
  while (getcmd(buf, sizeof(buf)) >= 0) {
    if (fork() == 0) {
      run_command(buf, 100, pcp);
    } else {
      int child_status;
      wait(&child_status);
      if (child_status == 2) {
        char dirName[100];
        close(pcp[1]);
        if (read(pcp[0], dirName, sizeof(dirName)) < 0) {
          fprintf(2, "Error: Cannot read pipe\n");
        } else if (chdir(dirName) < 0) {
          fprintf(2, "Error: Cannot change to directory %s\n", dirName);
        }
        close(pcp[0]);
      }
    }
  }
  exit(0);
}
