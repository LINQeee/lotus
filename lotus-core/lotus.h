#ifndef LOTUS_H
#define LOTUS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#define LOTUS_EXPORT __declspec(dllexport)
#define LOTUS_CALL __cdecl
#else
#define LOTUS_EXPORT __attribute__((visibility("default")))
#define LOTUS_CALL
#endif

#define LOTUS_COUNT(...)                                                       \
  (sizeof((LotusType[]){__VA_ARGS__}) / sizeof(LotusType))

#define LOTUS_FUNCTION(function, return_type, ...)                             \
  reg(&(LotusFunctionInfo){.name = #function,                                  \
                           .returnType = return_type,                          \
                           .argc = LOTUS_COUNT(__VA_ARGS__),                   \
                           .args = {__VA_ARGS__},                              \
                           .variadic = false,                                  \
                           .fn = function})

#define LOTUS_VARIADIC(function, return_type, ...)                             \
  reg(&(LotusFunctionInfo){.name = #function,                                  \
                           .returnType = return_type,                          \
                           .argc = LOTUS_COUNT(__VA_ARGS__),                   \
                           .args = {__VA_ARGS__},                              \
                           .variadic = true,                                   \
                           .fn = function})

#define LOTUS_NATIVE_LIBRARY                                                   \
  LOTUS_EXPORT void LOTUS_CALL lotusRegister(RegisterFunction reg)

typedef enum {
  LOTUS_I32,
  LOTUS_I64,
  LOTUS_F32,
  LOTUS_F64,
  LOTUS_BOOL,
  LOTUS_STRING,
  LOTUS_VOID,
  LOTUS_ANY
} LotusType;

typedef struct {
  LotusType type;

  union {
    int64_t i64;
    double f64;
    void *ptr;
  } value;
} LotusValue;

typedef struct {
  const char *name;
  bool variadic;
  LotusType returnType;
  int argc;
  LotusType args[32];
  void *fn;
} LotusFunctionInfo;

typedef void(LOTUS_CALL *RegisterFunction)(const LotusFunctionInfo *info);

#endif
