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

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

struct cmd {
  char *name;
  int argc;
  char **argv;
};

struct cmd_line {
  struct cmd **cmds;
  int count;
};

void greeting() {
  // "orsh-0.1$ "
  printf(ANSI_COLOR_RED "o" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_GREEN "r" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_YELLOW "s" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_BLUE "h" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_MAGENTA "-" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_CYAN "0" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_RED "." ANSI_COLOR_RESET);
  printf(ANSI_COLOR_GREEN "1" ANSI_COLOR_RESET);
  printf(ANSI_COLOR_YELLOW "$ " ANSI_COLOR_RESET);
}

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
  // printf("!{%s}!", comm);
  int flag = 0;
  cmd->argv = malloc(1 * sizeof(char *));
  cmd->argc = 0;

  char *start, *end;
  start = comm;
  end = comm;

  while (*start != '\0') {
    while (*start == ' ') start++;
    if (*start == '\0') break;
    end = start;

    if (*end == '\"') {
      start++;
      end++;
      while (*end != '\"') {
        end++;
      }
      end--;
    } else if (*end == '\'') {
      start++;
      end++;
      while (*end != '\'') {
        end++;
      }
      end--;
    } else if (*end == '>') {
      if (*(end + 1) == '>') {
        end += 1;
      }
    } else if (((*end == '|'))) {
    } else {
      while (((*end != ' ') && (*end != '>') && (*end != '|')) && (*end)) end++;
      end--;
    }

    char *token = malloc(sizeof(char) * (end - start + 2));
    memcpy(token, start, end - start + 1);
    token[end - start + 1] = '\0';

    if (((end[1] == '\"') && (*(start - 1) == '\"')) ||
        ((end[1] == '\'') && (*(start - 1) == '\''))) {
      if (end[2])
        start = end + 2;
      else
        *start = '\0';
    } else
      start = end + 1;

    printf("{%s}", token);

    if (!flag) {
      cmd->name = token;
      flag = 1;
    } else {
      cmd->argv[cmd->argc] = token;
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
    greeting();

    char *buff = NULL;
    int read;
    unsigned long int len;
    read = getline(&buff, &len, stdin);
    if (-1 == read) printf("No line read...\n");
    if (*buff == '\n') continue;

    string_trim_inplace(buff);
    delete_comments(buff);

    struct cmd_line line = parser(buff);

    int **fd;
    if (line.count > 1) {
      fd = malloc(line.count * sizeof(int *));
      for (int i = 0; i < line.count; i++) {
        fd[i] = malloc(2 * sizeof(int));
        pipe(fd[i]);
      }
    }

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
        if (line.count > 1) {
          pid_t child_pid = fork();
          if (child_pid == 0) {
            if (n == 0) {
              dup2(fd[n][1], STDOUT_FILENO);
              close(fd[n][0]);
              close(fd[n][1]);
            } else if (n == line.count - 1) {
              dup2(fd[n - 1][0], STDIN_FILENO);
              close(fd[n - 1][0]);
              close(fd[n - 1][1]);
            } else {
              dup2(fd[n - 1][0], STDIN_FILENO);
              dup2(fd[n][1], STDOUT_FILENO);
              close(fd[n][0]);
              close(fd[n][1]);
              close(fd[n - 1][0]);
              close(fd[n - 1][1]);
            }
            execvp(line.cmds[n]->name, cmd);
          }

          if (n > 0) {
            close(fd[n - 1][0]);
            close(fd[n - 1][1]);
          }

          waitpid(child_pid, NULL, 0);
        } else {
          pid_t child_pid = fork();
          if (child_pid == 0) execvp(line.cmds[n]->name, cmd);
          waitpid(child_pid, NULL, 0);
        }
        free(cmd);
      }
      //
    }

    if (line.count > 1) {
      for (int i = 0; i < line.count; i++) {
        free(fd[i]);
      }
      free(fd);
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
