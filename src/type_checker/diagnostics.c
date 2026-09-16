#include "diagnostics.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *findLineStart(const char *source, size_t targetLine) {
    const char *lineStart = source;
    size_t line = 1;

    while (*lineStart && line < targetLine) {
        if (*lineStart == '\n') line++;

        lineStart++;
    }

    return lineStart;
}

static size_t lineLength(const char *lineStart) {
    size_t length = 0;

    while (lineStart[length] != '\0' && lineStart[length] != '\n') length++;

    return length;
}

static void printLineNumber(size_t line, size_t width) {
    fprintf(stderr, "%*zu | ", (int)width, line);
}

static size_t digits(size_t value) {
    size_t result = 1;

    while (value >= 10) {
        value /= 10;
        result++;
    }

    return result;
}

void diagnosticError(const char *scriptContent, SourceLocation location, const char *format, ...) {
    fprintf(stderr, "error: ");

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");

    fprintf(stderr, " --> %s:%zu:%zu\n", location.file, location.line, location.column);

    const char *lineStart = findLineStart(scriptContent, location.line);

    if (!lineStart) exit(EXIT_FAILURE);

    size_t length = lineLength(lineStart);

    printLineNumber(location.line, digits(location.line));

    fwrite(lineStart, 1, length, stderr);
    fprintf(stderr, "\n");

    size_t numberWidth = digits(location.line);

    fprintf(stderr, "%*s | ", (int)numberWidth, "");

    for (size_t i = 1; i < location.column; i++) fputc(' ', stderr);

    size_t markerLength = location.length;

    if (markerLength == 0) markerLength = 1;

    if (location.column > length + 1) {
        markerLength = 1;
    } else {
        size_t available = length - (location.column - 1);
        if (markerLength > available) markerLength = available;
        if (markerLength == 0) markerLength = 1;
    }

    for (size_t i = 0; i < markerLength; i++) fputc('^', stderr);

    fputc('\n', stderr);

    exit(EXIT_FAILURE);
}
