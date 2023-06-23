#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libcoro.h"

void merge(int arr[], int l, int m, int r) {
  int i, j, k;
  int n1 = m - l + 1;
  int n2 = r - m;

  int L[n1], R[n2];

  for (i = 0; i < n1; i++) L[i] = arr[l + i];
  for (j = 0; j < n2; j++) R[j] = arr[m + 1 + j];

  i = 0;
  j = 0;
  k = l;
  while (i < n1 && j < n2) {
    if (L[i] <= R[j]) {
      arr[k] = L[i];
      i++;
    } else {
      arr[k] = R[j];
      j++;
    }
    k++;
  }

  while (i < n1) {
    arr[k] = L[i];
    i++;
    k++;
  }

  while (j < n2) {
    arr[k] = R[j];
    j++;
    k++;
  }
}

void mergeSort(int arr[], int l, int r) {
  if (l < r) {
    int m = l + (r - l) / 2;

    mergeSort(arr, l, m);
    coro_yield();
    mergeSort(arr, m + 1, r);

    merge(arr, l, m, r);
  }
}

void mergeArrays(int arr1[], int arr2[], int n1, int n2, int arr3[]) {
  int i = 0, j = 0, k = 0;

  while (i < n1 && j < n2) {
    if (arr1[i] < arr2[j])
      arr3[k++] = arr1[i++];
    else
      arr3[k++] = arr2[j++];
  }

  while (i < n1) arr3[k++] = arr1[i++];

  while (j < n2) arr3[k++] = arr2[j++];
}

int countFileSize(char *fileName) {
  int temp;
  int count = 0;
  FILE *file = fopen(fileName, "r");
  if (file == NULL) {
    printf("Could not open specified file");
    return -1;
  }

  while (fscanf(file, "%d", &temp) == 1) {
    count++;
  }
  fclose(file);
  return count;
}

typedef struct func_arg {
  int *arrToSort;
  int arrSize;
  char *fileToSort;
  struct timespec *start;
} Func_arg;

static int coroutine_func_f(void *context) {
  //   struct coro *this = coro_this();
  struct func_arg *fa = (struct func_arg *)context;
  FILE *fp = fopen(fa->fileToSort, "r");

  for (int i = 0; i < fa->arrSize; i++) {
    fscanf(fp, "%d ", &(fa->arrToSort)[i]);
  }

  mergeSort(fa->arrToSort, 0, fa->arrSize - 1);

  // printf("%s", fa->fileToSort);
  // for (int i = 0; i < 20; i++) {
  //   printf("%d ", fa->arrToSort[i]);
  // }
  fclose(fp);
  return 0;
}

int main(int argc, char **argv) {
  struct timespec start, end;
  long long elapsed_sec, elapsed_nsec;
  clock_gettime(CLOCK_MONOTONIC, &start);

  if (argc <= 1) {
    printf("No files to sort.\n");
    return EXIT_FAILURE;
  }

  int coroNum = atoi(argv[1]);
  if (coroNum == 0) {
    printf("Enter a proper number of coroutines.\n");
    return EXIT_FAILURE;
  }

  coro_sched_init();
  int **arrs = (int **)calloc((argc - 2), sizeof(int *));
  struct func_arg *fa = malloc((argc - 2) * sizeof(struct func_arg));

  int *sizes = malloc((argc - 2) * sizeof(int));

  for (int i = 0; i < argc - 2; ++i) {
    sizes[i] = countFileSize(argv[i + 2]);
    *(arrs + i) = (int *)calloc(sizes[i], sizeof(int));
    fa[i] = (Func_arg){*(arrs + i), sizes[i], argv[i + 2], &start};
    coro_new(coroutine_func_f, &(fa[i]));
  }

  struct coro *c;
  while ((c = coro_sched_wait()) != NULL) {
    printf("Finished %d\n", coro_status(c));
    // print time here
    coro_delete(c);
  }

  for (int i = 0; i < argc - 2 - 1; ++i) {
    int *ans = malloc((sizes[i] + sizes[i + 1]) * sizeof(int));
    mergeArrays(*(arrs + i), *(arrs + i + 1), sizes[i], sizes[i + 1], ans);
    free(*(arrs + i));
    free(*(arrs + i + 1));
    *(arrs + i + 1) = ans;
    sizes[i + 1] += sizes[i];
  }

  FILE *file = fopen("output.txt", "w");
  if (file == NULL) {
    printf("Error opening the file.\n");
    return 1;
  }

  for (int i = 0; i < sizes[argc - 2 - 1]; i++) {
    fprintf(file, "%d\n", arrs[argc - 2 - 1][i]);
  }

  fclose(file);

  free(arrs[argc - 2 - 1]);
  free(arrs);
  free(fa);
  free(sizes);

  clock_gettime(CLOCK_MONOTONIC, &end);
  elapsed_sec = end.tv_sec - start.tv_sec;
  elapsed_nsec = end.tv_nsec - start.tv_nsec;
  if (elapsed_nsec < 0) {
    elapsed_sec--;
    elapsed_nsec += 1000000000;
  }
  printf("Total elapsed time: %lld seconds %lld milliseconds\n", elapsed_sec,
         elapsed_nsec / 1000000);

  return 0;
}
