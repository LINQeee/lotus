#include "lotus.h"
#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// WARNING
// BAD CODE HERE MIGHT CRASH THE SHIT OUT OF PROGRAM
// WARNING
// CHANGE ANYTHING AT YOUR OWN RISK
// Nah, I'm jk, do whatever u want

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
      const LotusString *str = val.value.ptr;
      printf("%s", str->data);
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

LotusString *_concat(LotusString *str1, LotusString *str2) {
  LotusString *result = malloc(sizeof(LotusString));
  result->length = str1->length + str2->length;
  char *data = malloc(result->length + 1);

  memcpy(data, str1->data, str1->length);
  memcpy(data + str1->length, str2->data, str2->length);

  data[result->length] = '\0';
  result->data = data;
  return result;
}

int randomNum(int min, int max) {
  srand(time(NULL));
  return rand() % (max - min + 1) + min;
}

LotusString *input() {
  LotusString *str = malloc(sizeof(LotusString));

  size_t capacity = 16;
  str->data = malloc(capacity);

  size_t length = 0;
  int c;

  while ((c = getchar()) != '\n' && c != EOF) {
    if (length + 1 >= capacity) {
      capacity *= 2;
      str->data = realloc(str->data, capacity);
    }

    str->data[length++] = (char)c;
  }

  str->data[length] = '\0';
  str->length = length;

  return str;
}

LOTUS_NATIVE_LIBRARY {
  LOTUS_FUNCTION(randomNum, LOTUS_I32, LOTUS_I32, LOTUS_I32);

  LOTUS_FUNCTION(toBool, LOTUS_BOOL, LOTUS_ANY);
  LOTUS_FUNCTION(sumFloat, LOTUS_F64, LOTUS_F64, LOTUS_F64);

  LOTUS_VARIADIC(print, LOTUS_VOID, LOTUS_ANY);
  LOTUS_VARIADIC(addMany, LOTUS_I32, LOTUS_I32);

  LOTUS_FUNCTION(input, LOTUS_STRING);
  LOTUS_FUNCTION(_concat, LOTUS_STRING, LOTUS_STRING, LOTUS_STRING);
}
