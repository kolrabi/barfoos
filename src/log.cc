#include <cstdarg>
#include <cstdio>

#include "log.h"

void Log(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int length = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);

  char str[length+1];

  va_start(ap, fmt);
  length = vsnprintf(str, length+1, fmt, ap);
  va_end(ap);

  fputs(str, stderr);
  fflush(stderr);
  
  
  FILE *f = fopen("log.txt", "a");
  fputs(str, f);
  fclose(f);
  
}
