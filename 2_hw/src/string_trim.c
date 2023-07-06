#include <ctype.h>
#include <string.h>
#include <sys/types.h>

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

char *string_trim(char *s) {
  char *original = s;
  size_t len = 0;

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