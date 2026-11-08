#include <stdbool.h>
#include "../lotus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "functions.h"

bool toBool(const LotusValue *value) {
    switch (value->type) {
        case LOTUS_I32:
        case LOTUS_I64: return value->value.i64 > 0;
        case LOTUS_F32:
        case LOTUS_F64: return value->value.f64 > 0;
        case LOTUS_BOOL: return value->value.i64 != 0;
        case LOTUS_STRING: {
            const LotusString *str = value->value.ptr;
            if (strcmp(str->data, "true") == 0) return true;
            if (strcmp(str->data, "false") == 0) return false;
            fprintf(stderr, "Illegal value to convert string to bool: %s", str->data);
            return false;
        }
        default: return false;
    }
}

int toInt(LotusString *str) {
    return atoi(str->data);
}
