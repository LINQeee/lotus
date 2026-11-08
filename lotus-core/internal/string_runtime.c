#include "../lotus.h"
#include <stdlib.h>
#include <string.h>

#include "string_runtime.h"
#include "../utils/string_utils.h"

void _stringRetain(LotusString *str) {
    if (str->references) str->references++;
}

void _stringRelease(LotusString *str) {
    // don't release static strings
    if (!str->references) return;
    if (--str->references) return;
    free(str->data);
    free(str);
}

bool _stringEquals(const LotusString *left, const LotusString *right) {
    return left->length == right->length && memcmp(left->data, right->data, left->length) == 0;
}

LotusString *_concat(LotusString *str1, LotusString *str2) {
    LotusString *result = lotusStringCreate(str1->length + str2->length);
    char *data = result->data;

    memcpy(data, str1->data, str1->length);
    memcpy(data + str1->length, str2->data, str2->length);

    data[result->length] = '\0';
    result->data = data;
    return result;
}
