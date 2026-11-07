#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "lotus.h"

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

double sumFloat(double a, double b) {
    return a + b;
}

void print(int count, ...) {
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        LotusValue val = va_arg(args, LotusValue);
        switch (val.type) {
            case LOTUS_BOOL: printf("%s", val.value.i64 == 0 ? "false" : "true"); break;
            case LOTUS_F32:
            case LOTUS_F64: printf("%f", val.value.f64); break;
            case LOTUS_I64:
            case LOTUS_I32: printf("%lld", (long long)val.value.i64); break;
            case LOTUS_STRING: {
                const LotusString *str = val.value.ptr;
                printf("%s", str->data);
                break;
            }
        }
    }
    va_end(args);
    printf("\n");
}

bool toBool(const LotusValue value) {
    switch (value.type) {
        case LOTUS_I32:
        case LOTUS_I64: return value.value.i64 > 0;
        case LOTUS_F32:
        case LOTUS_F64: return value.value.f64 > 0;
        case LOTUS_BOOL: return value.value.i64 != 0;
        case LOTUS_STRING: {
            const LotusString *str = value.value.ptr;
            if (strcmp(str->data, "true") == 0) return true;
            if (strcmp(str->data, "false") == 0) return false;
            fprintf(stderr, "Illegal value to convert string to bool: %s", str->data);
            return false;
        }
        default: return false;
    }
}

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

int randomNum(int min, int max) {
    srand(time(NULL));
    return rand() % (max - min + 1) + min;
}

LotusString *input() {
    size_t capacity = 16;
    LotusString *str = lotusStringCreate(capacity - 1);

    size_t length = 0;
    int c;

    while ((c = getchar()) != '\n' && c != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char *data = realloc(str->data, capacity);
            if (!data) {
                _stringRelease(str);
                fprintf(stderr, "Cannot grow input string\n");
                exit(EXIT_FAILURE);
            }
            str->data = data;
        }

        str->data[length++] = (char)c;
    }

    str->data[length] = '\0';
    str->length = length;

    return str;
}

int toInt(LotusString *str) {
    return atoi(str->data);
}

void hello(LotusString *str, int age) {
    printf("Hello %s! With age %d\n", str->data, age);
}

LOTUS_NATIVE_LIBRARY {
    LOTUS_FUNCTION(hello, LOTUS_VOID, LOTUS_STRING, LOTUS_I32);

    LOTUS_FUNCTION(randomNum, LOTUS_I32, LOTUS_I32, LOTUS_I32);

    LOTUS_FUNCTION(toBool, LOTUS_BOOL, LOTUS_ANY);
    LOTUS_FUNCTION(sumFloat, LOTUS_F64, LOTUS_F64, LOTUS_F64);

    LOTUS_VARIADIC(print, LOTUS_VOID, LOTUS_ANY);
    LOTUS_VARIADIC(addMany, LOTUS_I32, LOTUS_I32);

    LOTUS_FUNCTION(toInt, LOTUS_I32, LOTUS_STRING);

    LOTUS_FUNCTION(input, LOTUS_STRING);
    LOTUS_FUNCTION(_concat, LOTUS_STRING, LOTUS_STRING, LOTUS_STRING);
    LOTUS_FUNCTION(_stringEquals, LOTUS_BOOL, LOTUS_STRING, LOTUS_STRING);
    LOTUS_FUNCTION(_stringRetain, LOTUS_VOID, LOTUS_STRING);
    LOTUS_FUNCTION(_stringRelease, LOTUS_VOID, LOTUS_STRING);
}
