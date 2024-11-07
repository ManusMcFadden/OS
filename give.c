#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

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
  int p[2];

  int i = 0;
  for (; i < nbuf; i++) {
    if (buf[i] == '<') {
      redirection_left = 1;
      buf[i] = '\0';
      file_name_l = &buf[i + 1];
      while (*file_name_l == ' ') file_name_l++; // Skip spaces after '<'
      
      // Find the end of the filename by looking for the first space or null character
      for (int j = 0; file_name_l[j] != '\0' && file_name_l[j] != ' ' && file_name_l[j] != '\n'; j++) {
        if (file_name_l[j] == '\0' || file_name_l[j] == ' ') {
          file_name_l[j] = '\0';
          break;
        }
      }
      
      if (*file_name_l == '\0') {
        fprintf(2, "Error: Need a filename after <\n");
        return;
      }
      break;
    }
    if (buf[i] == '>') {
      redirection_right = 1;
      buf[i] = '\0';
      file_name_r = &buf[i + 1];
      while (*file_name_r == ' ') file_name_r++; // Skip spaces after '>'

      // Find the end of the filename by looking for the first space or null character
      for (int j = 0; file_name_r[j] != '\0' && file_name_r[j] != ' ' && file_name_r[j] != '\n'; j++) {
        if (file_name_r[j] == '\0' || file_name_r[j] == ' ') {
          file_name_r[j] = '\0';
          break;
        }
      }

      if (*file_name_r == '\0') {
        fprintf(2, "Error: Need a filename after >\n");
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
  
  // The rest of the function remains the same...
}

int main(void) {
  static char buf[100];
  int pcp[2];
  pipe(pcp);

  while (getcmd(buf, sizeof(buf)) >= 0) {
    if (fork() == 0) {
      run_command(buf, 100, pcp);
    } else {
      int child_status;
      wait(&child_status);
      if (child_status == 2) {
        char dirName[100];
        close(pcp[1]);
        read(pcp[0], dirName, sizeof(dirName));
        close(pcp[0]);
        chdir(dirName);
      }
    }
  }
  exit(0);
}
