#include "../lotus.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "functions.h"
#include "../internal/string_runtime.h"
#include "../utils/string_utils.h"

static void prepareConsole(void) {
#ifdef _WIN32
    static bool configured;
    if (configured) return;
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    configured = true;
#endif
}

void print(int count, ...) {
    prepareConsole();
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        const LotusValue *val = va_arg(args, const LotusValue *);
        switch (val->type) {
            case LOTUS_BOOL: printf("%s", val->value.i64 == 0 ? "false" : "true"); break;
            case LOTUS_F32:
            case LOTUS_F64: printf("%f", val->value.f64); break;
            case LOTUS_I64:
            case LOTUS_I32: printf("%lld", (long long)val->value.i64); break;
            case LOTUS_STRING: {
                const LotusString *str = val->value.ptr;
                printf("%s", str->data);
                break;
            }
        }
    }
    va_end(args);
    printf("\n");
}

LotusString *input() {
    prepareConsole();
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
