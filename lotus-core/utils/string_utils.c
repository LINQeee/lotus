#include "../lotus.h"
#include <stdio.h>
#include <stdlib.h>

#include "string_utils.h"

LotusString *lotusStringCreate(size_t length) {
    LotusString *str = malloc(sizeof(LotusString));
    char *data = malloc(length + 1);
    if (!str || !data) {
        free(str);
        free(data);
        fprintf(stderr, "Cannot allocate string\n");
        exit(EXIT_FAILURE);
    }
    *str = (LotusString){length, data, 1};
    data[length] = '\0';
    return str;
}
