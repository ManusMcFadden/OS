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
      while (*file_name_l == ' ') file_name_l++;  // Skip initial spaces

      // Stop filename at the next special character
      for (char *p = file_name_l; *p; p++) {
        if (*p == ' ' || *p == '<' || *p == '>' || *p == '|' || *p == ';') {
          *p = '\0';
          break;
        }
      }
      continue;
    }

    if (buf[i] == '>') {
      redirection_right = 1;
      buf[i] = '\0';
      file_name_r = &buf[i + 1];
      while (*file_name_r == ' ') file_name_r++;  // Skip initial spaces

      // Stop filename at the next special character
      for (char *p = file_name_r; *p; p++) {
        if (*p == ' ' || *p == '<' || *p == '>' || *p == '|' || *p == ';') {
          *p = '\0';
          break;
        }
      }
      continue;
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
      exit(1);
    }
  }

  if (numargs == 0) {
    exit(1);
  }

  arguments[numargs] = 0;

  // Handle pipes, sequences, redirections, and execution (no changes here) ...

  // The rest of the code remains unchanged for handling pipes, sequences, 
  // redirections, and execution.

  if (pipe_cmd) {
    if (pipe(p) < 0) {
      fprintf(2, "Error: Could not make pipe\n");
      exit(1);
    }

    if (fork() == 0) {
      close(p[0]);
      close(1);
      dup(p[1]);
      close(p[1]);

      exec(arguments[0], arguments);
      fprintf(2, "Error: Could not execute %s\n", arguments[0]);
      exit(1);
    }

    close(p[1]);

    if (fork() == 0) {
      close(0);
      dup(p[0]);
      close(p[0]);

      char *rightCmd = &buf[i + 1];
      run_command(rightCmd, nbuf - (i + 1), pcp);
      exit(0);
    }

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
      fprintf(2, "Error: Couldn't read file: %s\n", file_name_l);
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
        fprintf(2, "Error: Couldn't write to file: %s\n", file_name_r);
        exit(1);
      }
      close(1);
      dup(fd);
      close(fd);
    } else {
      fprintf(2, "Error: No write file included\n");
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
      fprintf(2, "Error: Could not execute %s\n", arguments[0]);
      exit(1);
    } else {
      wait(0);
    }
  }
  exit(0);
}
