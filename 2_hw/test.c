#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char x = '';
  char *y = &x;

  printf("%d", *y == 'z');
}