#include "libcoro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILE_SIZE 10000

void merge(int arr[], int l, int m, int r) {
  int i, j, k;
  int n1 = m - l + 1;
  int n2 = r - m;

  /* create temp arrays */
  int L[n1], R[n2];

  /* Copy data to temp arrays L[] and R[] */
  for (i = 0; i < n1; i++)
    L[i] = arr[l + i];
  for (j = 0; j < n2; j++)
    R[j] = arr[m + 1 + j];

  /* Merge the temp arrays back into arr[l..r]*/
  i = 0; // Initial index of first subarray
  j = 0; // Initial index of second subarray
  k = l; // Initial index of merged subarray
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

  /* Copy the remaining elements of L[], if there
     are any */
  while (i < n1) {
    arr[k] = L[i];
    i++;
    k++;
  }

  /* Copy the remaining elements of R[], if there
     are any */
  while (j < n2) {
    arr[k] = R[j];
    j++;
    k++;
  }
}

/* l is for left index and r is the right index of the
   sub-array of arr to be sorted */
void mergeSort(int arr[], int l, int r) {
  if (l < r) {
    // Same as (l+r)/2, but avoids overflow for
    // large l and h
    int m = l + (r - l) / 2;

    // Sort first and second halves
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

    while (i < n1)
      arr3[k++] = arr1[i++];

    while (j < n2)
      arr3[k++] = arr2[j++];
}

struct func_arg {
  int *arrToSort;
  FILE *fileToSort;
  struct timespec *start;
};

static int coroutine_func_f(void *context) {
  //   struct coro *this = coro_this();
  struct func_arg *fa = (struct func_arg *)context;
  // FILE *fp = fa->fileToSort;
  // int *numArr = fa->arrToSort;

  for (int i = 0; i < FILE_SIZE; i++) {
    fscanf(fa->fileToSort, "%d ", &(fa->arrToSort)[i]);
  }

  mergeSort(fa->arrToSort, 0, FILE_SIZE - 1);

  for (int i = 0; i < FILE_SIZE; i++) {
    printf("%d ", fa->arrToSort[i]);
  }


  // if (fclose(fa->fileToSort)) { printf("error closing file."); exit(-1); }

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

  for (int i = 0; i < argc - 2; ++i) {
    FILE *fp = fopen(argv[i + 2], "r");
    if (!fp) {
      perror("File opening failed");
      return EXIT_FAILURE;
    }

    *(arrs + i) = (int *)calloc(FILE_SIZE, sizeof(int));
    struct func_arg fa = {*(arrs + i), fp, &start};
    coro_new(coroutine_func_f, &fa);
  }

  struct coro *c;
  while ((c = coro_sched_wait()) != NULL) {
    printf("Finished %d\n", coro_status(c));
    coro_delete(c);
  }

  for(int i = 0; i < FILE_SIZE; i++) {
    printf("%d ", (*arrs)[i]);
  }

  // for(int i = 0; i < argc - 2 - 1; ++i) {
  //   int* ans = malloc(FILE_SIZE * (i + 2) * sizeof(int));
  //   mergeArrays(*(arrs + i), *(arrs + i + 1), FILE_SIZE, FILE_SIZE, ans);
  //   free(*(arrs + i));
  //   free(*(arrs + i + 1));
  //   *(arrs + i + 1) = ans;
  // }

  // for (int i = 0; i < argc - 2; ++i) {
  //   free(*(arrs + i));
  // }
  FILE *file = fopen("output.txt", "w");
  if (file == NULL) {
      printf("Error opening the file.\n");
      return 1;
  }
  
  for (int i = 0; i < FILE_SIZE*(argc-2); i++) {
      fprintf(file, "%d\n", arrs[argc-2-1][i]);
  }
  
  fclose(file);


  free(arrs[argc - 2 - 1]);
  free(arrs);


  clock_gettime(CLOCK_MONOTONIC, &end);
  elapsed_sec = end.tv_sec - start.tv_sec;
  elapsed_nsec = end.tv_nsec - start.tv_nsec;
  if (elapsed_nsec < 0) {
    elapsed_sec--;
    elapsed_nsec += 1000000000;
  }
  printf("Total elapsed time: %lld seconds %lld milliseconds\n", elapsed_sec, elapsed_nsec/1000000);

  return 0;
}
