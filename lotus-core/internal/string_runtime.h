#ifndef LOTUS_INTERNAL_STRING_H
#define LOTUS_INTERNAL_STRING_H

#include <stdbool.h>

#include "../lotus.h"

LOTUS_EXPORT void _stringRetain(LotusString *str);
LOTUS_EXPORT void _stringRelease(LotusString *str);
LOTUS_EXPORT bool _stringEquals(const LotusString *left, const LotusString *right);
LOTUS_EXPORT LotusString *_concat(LotusString *left, LotusString *right);

#endif
