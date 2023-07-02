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

// int main() {
//   int x;
//   scanf("%d", &x);

//   int **fd = malloc(x * sizeof(int *));
//   for (int i = 0; i < x; i++) {
//     *(fd + i) = malloc(2 * sizeof(int));
//   }

//   fd[1][1] = 2;

//   printf("fd[1][1]: %d \n", fd[0][1]);

//   for (int i = 0; i < x; i++) {
//     free(*(fd + i));
//   }
//   free(fd);
// }

int main() { printf("{%d} \n", 5 / 2 + 1); }