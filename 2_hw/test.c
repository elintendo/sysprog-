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

int main() {
  char x[] = "hello man";
  char *start = &x[0];
  char *end = &x[4];

  char *token = malloc(sizeof(char) * (end - start + 2));
  memcpy(token, start, end - start + 1);
  token[end - start + 1] = '\0';

  printf("|%c|\n", token[5]);
}