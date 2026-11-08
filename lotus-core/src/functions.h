#ifndef LOTUS_FUNCTIONS_H
#define LOTUS_FUNCTIONS_H

#include <stdbool.h>

#include "../lotus.h"

LOTUS_EXPORT void print(int count, ...);
LOTUS_EXPORT LotusString *input(void);
LOTUS_EXPORT bool toBool(const LotusValue *value);
LOTUS_EXPORT int toInt(LotusString *str);
LOTUS_EXPORT int randomNum(int min, int max);

#endif
