#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <err.h>
#include "error.h"

void errorf(const char* format, ...)
{
  va_list argptr;
  char *message;
  va_start(argptr, format);
  vasprintf(&message, format, argptr);
  perror(message);
  free(message);
  va_end(argptr);
  exit(1);
}