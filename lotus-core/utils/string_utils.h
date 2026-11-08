#ifndef LOTUS_UTILS_STRING_H
#define LOTUS_UTILS_STRING_H

#include <stddef.h>

#include "../lotus.h"

// Creates an owned string with space for length bytes and a trailing NUL.
#ifndef _WIN32
__attribute__((visibility("hidden")))
#endif
LotusString *lotusStringCreate(size_t length);

#endif
