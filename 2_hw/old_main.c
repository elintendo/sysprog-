#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cmd {
  char *name;
  int argc;
  char **argv;
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
    // len = (size_t) (p - s);   // older errant code
    len = (size_t)(p - s + 1);  // Thanks to @theriver
  }

  return (s == original) ? s : memmove(original, s, len + 1);
}

void commandParser(struct cmd *cmd, char *comm) {
  printf("hi from com parser\n");

  char *rest = NULL;
  char *token;

  int flag = 0;
  cmd->argv = malloc(1 * sizeof(char *));
  cmd->argc = 0;

  for (token = strtok_r(comm, " ", &rest); token != NULL;
       token = strtok_r(NULL, " ", &rest)) {
    printf("com token: {%s}\n", token);
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

void parser(struct cmd **cmds, int *i, char *buff) {
  char *copy = strdup(buff);

  char *rest = NULL;
  char *token;

  for (token = strtok_r(copy, "|", &rest); token != NULL;
       token = strtok_r(NULL, "|", &rest)) {
    printf("token: {%s}\n", token);

    cmds[*i] = malloc(1 * sizeof(struct cmd));
    commandParser(cmds[*i], strdup(token));

    *i += 1;

    struct cmd **new_cmds = realloc(cmds, (*i + 1) * sizeof(struct cmd *));
    if (new_cmds == NULL) {
      exit(1);
    }
    cmds = new_cmds;
  }

  free(copy);
}

int main(void) {
  char *buff = NULL;
  int read;
  unsigned long int len;
  read = getline(&buff, &len, stdin);
  if (-1 == read) printf("No line read...\n");

  struct cmd **cmds = malloc(1 * sizeof(struct cmd *));
  int i = 0;
  parser(cmds, &i, string_trim_inplace(buff));

  // printf("!!%s\n", (cmds[2]->argv[2]));

  for (int k = 0; k < i; k++) {
    free(cmds[k]->name);
    for (int m = 0; m < cmds[k]->argc; m++) {
      free(cmds[k]->argv[m]);
    }
    free(cmds[k]->argv);
    free(cmds[k]);
  }
  free(cmds);
  free(buff);
  return 0;
}