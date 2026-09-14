#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "../lexer/lexer.h"

typedef struct {
    const char *source;
    const char *file;
} DiagnosticContext;

void diagnosticError(const char *scriptContent, SourceLocation location, const char *format, ...);

#endif
