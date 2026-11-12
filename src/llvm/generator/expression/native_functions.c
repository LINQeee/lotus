#include "native_functions.h"

#include <stdbool.h>
#include <string.h>

#include "../../llvm.h"
#include "../../../type_checker/type_checker.h"
#include "../../lotus_types.h"
#include <llvm-c/Core.h>
#include <llvm-c/Support.h>
#include <llvm-c/Types.h>

LLVMTypeRef lotusValueLLVMType() {
    static LLVMTypeRef type = NULL;

    if (type) return type;

    LLVMTypeRef fields[] = {cg->i32Type, cg->i64Type};

    type = LLVMStructCreateNamed(cg->context, "LotusValue");

    LLVMStructSetBody(type, fields, 2, false);

    return type;
}

LLVMTypeRef convertType(const LotusType type) {
    if (isMemoryType(type) || type == LOTUS_ARRAY) return LLVMPointerType(cg->i8Type, 0);
    switch (type) {
        case LOTUS_I32: return cg->i32Type;
        case LOTUS_I64: return cg->i64Type;
        case LOTUS_BOOL: return cg->boolType;
        case LOTUS_F32: return cg->f32Type;
        case LOTUS_F64: return cg->f64Type;
        case LOTUS_STRING: return LLVMPointerType(cg->i8Type, 0);
        case LOTUS_VOID: return cg->voidType;
        case LOTUS_ANY: return LLVMPointerType(lotusValueLLVMType(), 0);
    }
    return cg->voidType;
}

void prepareNativeFunction(const LotusFunctionInfo *info) {
    LLVMNativeFunction *native = &cg->natives[cg->nativeCount++];

    strcpy(native->name, info->name);

    native->returnType = info->returnType;
    native->argc = info->argc;
    native->variadic = info->variadic;
    native->fn = info->fn;
    LLVMAddSymbol(info->name, info->fn);

    for (int i = 0; i < info->argc; i++) native->args[i] = info->args[i];

    LLVMTypeRef params[32];
    if (info->variadic) params[0] = cg->i32Type;
    for (int i = 0; i < info->argc; i++) params[info->variadic ? i + 1 : i] = convertType(info->args[i]);

    LLVMTypeRef fnType = LLVMFunctionType(convertType(info->returnType), params, info->argc, info->variadic);
    native->fnType = fnType;

    native->llvmFunction = LLVMAddFunction(cg->module, native->name, fnType);

    typeCheckerAddNativeFunction(info->name, info->returnType, info->args, info->argc, info->variadic);
}
