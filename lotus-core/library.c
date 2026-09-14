#include "lotus.h"
#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int addMany(int count, ...) {
  va_list args;
  va_start(args, count);

  int result = 0;
  for (int i = 0; i < count; i++) {
    result += va_arg(args, int);
  }
  va_end(args);
  return result;
}

double sumFloat(double a, double b) { return a + b; }

void print(int count, ...) {
  va_list args;
  va_start(args, count);
  for (int i = 0; i < count; i++) {
    LotusValue val = va_arg(args, LotusValue);
    switch (val.type) {
    case LOTUS_BOOL:
      printf("%s", val.value.i64 == 0 ? "false" : "true");
      break;
    case LOTUS_F32:
    case LOTUS_F64:
      printf("%f", val.value.f64);
      break;
    case LOTUS_I64:
    case LOTUS_I32:
      printf("%ld", val.value.i64);
      break;
    case LOTUS_STRING:
      printf("%s", (char *)val.value.ptr);
      break;
    }
  }
  printf("\n");
}

bool toBool(const LotusValue value) {
  switch (value.type) {
  case LOTUS_I32:
  case LOTUS_I64:
    return value.value.i64 > 0;
  case LOTUS_F32:
  case LOTUS_F64:
    return value.value.f64 > 0;
  case LOTUS_BOOL:
    return value.value.i64 != 0;
  case LOTUS_STRING:
    if (strcmp(value.value.ptr, "true") == 0)
      return true;
    if (strcmp(value.value.ptr, "false") == 0)
      return false;
    fprintf(stderr, "Illegal value to convert string to bool: %s",
            (char *)value.value.ptr);
    return false;
  default:
    return false;
  }
}

int randomNum(int min, int max) {
  srand(time(NULL));
  return rand() % (max - min + 1) + min;
}

LOTUS_NATIVE_LIBRARY {
  LOTUS_FUNCTION(randomNum, LOTUS_I32, LOTUS_I32, LOTUS_I32);

  LOTUS_FUNCTION(toBool, LOTUS_BOOL, LOTUS_ANY);
  LOTUS_FUNCTION(sumFloat, LOTUS_F64, LOTUS_F64, LOTUS_F64);

  LOTUS_VARIADIC(print, LOTUS_VOID, LOTUS_ANY);
  LOTUS_VARIADIC(addMany, LOTUS_I32, LOTUS_I32);
}
