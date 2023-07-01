#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct cmd {
  char *name;
  int argc;
  char **argv;
};

struct cmd_line {
  struct cmd **cmds;
  int count;
};

char *string_trim_inplace(char *s) {
  char *original = s;
  size_t len = 0;

  while (isspace((unsigned char)*s)) {
    s++;
  }
  if (*s) {
    char *p = s;
    while (*p) p++;
    while (isspace((unsigned char)*(--p)))
      ;
    p[1] = '\0';
    len = (size_t)(p - s + 1);
  }

  return (s == original) ? s : memmove(original, s, len + 1);
}

void delete_comments(char *s) {
  char *p = s;
  while ((*p != '#') && (*p)) p++;
  if (*p) {
    *p = '\0';
  } else
    return;
}

void commandParser(struct cmd *cmd, char *comm) {
  // printf("hi from com parser\n");

  char *rest = NULL;
  char *token;

  int flag = 0;
  cmd->argv = malloc(1 * sizeof(char *));
  cmd->argc = 0;

  for (token = strtok_r(comm, " ", &rest); token != NULL;
       token = strtok_r(NULL, " ", &rest)) {
    // printf("subtoken: {%s}\n", token);
    // if (*token == '\"') {
    //   while (token != '\"') {
    //     token++;
    //   }
    // }
    if (!flag) {
      cmd->name = strdup(token);
      flag = 1;
    } else {
      cmd->argv[cmd->argc] = strdup(token);
      cmd->argc += 1;

      char **new_argv = realloc(cmd->argv, sizeof(char *) * (cmd->argc + 1));
      if (new_argv == NULL) {
        exit(1);
      }
      cmd->argv = new_argv;
    }
  }

  free(comm);
}

struct cmd_line parser(char *buff) {
  struct cmd_line res;
  res.count = 0;
  res.cmds = malloc(sizeof(*res.cmds));
  char *copy = strdup(buff);

  char *rest = NULL;
  char *token;

  for (token = strtok_r(copy, "|", &rest); token != NULL;
       token = strtok_r(NULL, "|", &rest)) {
    // printf("token: {%s}\n", token);

    res.cmds[res.count] = malloc(sizeof(**res.cmds));
    commandParser(res.cmds[res.count], strdup(token));
    ++res.count;

    res.cmds = realloc(res.cmds, (res.count + 1) * sizeof(*res.cmds));
    assert(res.cmds != NULL);
  }
  free(copy);
  return res;
}

void cd_command(struct cmd_line *line) {
  if (line->cmds[0]->argc > 0) {
    if (chdir(line->cmds[0]->argv[0]) == -1) {
      printf("\nCannot change directory.\n");
    }
  } else {
    if (chdir(getenv("HOME")) == -1) {
      printf("\nCannot change directory.\n");
    }
  }
}

int main(void) {
  while (1) {
    char *buff = NULL;
    int read;
    unsigned long int len;
    read = getline(&buff, &len, stdin);
    if (-1 == read) printf("No line read...\n");
    if (*buff == '\n') continue;

    string_trim_inplace(buff);
    delete_comments(buff);

    struct cmd_line line = parser(buff);

    //

    for (int n = 0; n < line.count; n++) {
      char **cmd = malloc((line.cmds[n]->argc + 2) * sizeof(char *));
      cmd[0] = line.cmds[n]->name;
      for (int i = 1; i < line.cmds[n]->argc + 1; i++) {
        cmd[i] = line.cmds[n]->argv[i - 1];
      }
      cmd[line.cmds[n]->argc + 1] = (char *)0;
      // printf("{%s}\n", cmd[0]);
      char *secondLast = line.cmds[n]->argv[line.cmds[n]->argc - 2];

      if (!strcmp(line.cmds[n]->name, "cd")) {
        cd_command(&line);
        free(cmd);
      } else if (!strcmp(line.cmds[n]->name, "exit")) {
      } else if ((line.cmds[n]->argc >= 2) &&
                 (!strcmp(secondLast, ">") || !strcmp(secondLast, ">>"))) {
        pid_t child_pid = fork();
        if (child_pid == 0) {
          char *file_name = line.cmds[n]->argv[line.cmds[n]->argc - 1];
          int file;

          if (!strcmp(secondLast, ">"))
            file = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
          else
            file = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0777);

          dup2(file, STDOUT_FILENO);
          close(file);
          cmd[line.cmds[n]->argc] = (char *)0;
          cmd[line.cmds[n]->argc - 1] = (char *)0;
          execvp(line.cmds[n]->name, cmd);
        }
        free(cmd);
      } else {
        // printf("{%s}\n", cmd[0]);
        pid_t child_pid = fork();
        if (child_pid == 0) {
          execvp(line.cmds[n]->name, cmd);
        }
        free(cmd);
        waitpid(child_pid, NULL, 0);
      }
      //
    }

    for (int k = 0; k < line.count; k++) {
      free(line.cmds[k]->name);
      for (int m = 0; m < line.cmds[k]->argc; m++) {
        free(line.cmds[k]->argv[m]);
      }
      free(line.cmds[k]->argv);
      free(line.cmds[k]);
    }
    free(line.cmds);
    free(buff);
  }
}
