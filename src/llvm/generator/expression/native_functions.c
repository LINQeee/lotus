#include "native_functions.h"

#include <stdbool.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/Support.h>
#include <llvm-c/Types.h>

#include "../../../utils/cutils.h"
#include "../../llvm.h"
#include "../../../type_checker/type_checker.h"
#include "../../types.h"
#include "lotus.h"

Type *fromNativeType(NativeType type) {
    switch (type) {
        case LOTUS_I32: return intType;
        case LOTUS_I64: return longType;
        case LOTUS_F32: return floatType;
        case LOTUS_F64: return doubleType;
        case LOTUS_BOOL: return boolType;
        case LOTUS_STRING: return stringType;
        case LOTUS_VOID: return voidType;
        case LOTUS_ANY: return anyType;
        case LOTUS_FUNCTION: return functionType;
        case LOTUS_UNKNOWN: return unknownType;
        case LOTUS_ARRAY: return anyArrayType;
        case LOTUS_I32_ARRAY: return toArrayType(intType);
        case LOTUS_I64_ARRAY: return toArrayType(longType);
        case LOTUS_F32_ARRAY: return toArrayType(floatType);
        case LOTUS_F64_ARRAY: return toArrayType(doubleType);
        case LOTUS_BOOL_ARRAY: return toArrayType(boolType);
        case LOTUS_STRING_ARRAY: return toArrayType(stringType);
    }
    exitWithError("Unknown native type: %d", type);
}
NativeType toNativeType(Type *type) {
    if (isArrayType(type)) {
        switch (type->elementType->kind) {
            case TYPE_I32: return LOTUS_I32_ARRAY;
            case TYPE_I64: return LOTUS_I64_ARRAY;
            case TYPE_F32: return LOTUS_F32_ARRAY;
            case TYPE_F64: return LOTUS_F64_ARRAY;
            case TYPE_BOOL: return LOTUS_BOOL_ARRAY;
            case TYPE_STRING: return LOTUS_STRING_ARRAY;
            default: break;
        }
    }
    switch (type->kind) {
        case TYPE_I32: return LOTUS_I32;
        case TYPE_I64: return LOTUS_I64;
        case TYPE_F32: return LOTUS_F32;
        case TYPE_F64: return LOTUS_F64;
        case TYPE_BOOL: return LOTUS_BOOL;
        case TYPE_STRING: return LOTUS_STRING;
        case TYPE_VOID: return LOTUS_VOID;
        case TYPE_ANY: return LOTUS_ANY;
        case TYPE_FUNCTION: return LOTUS_FUNCTION;
        case TYPE_UNKNOWN: return LOTUS_UNKNOWN;
        case TYPE_ANY_ARRAY: return LOTUS_ARRAY;
        default: exitWithError("Type %s has no native representation", typeName(type));
    }
}

LLVMTypeRef lotusValueLLVMType() {
    static LLVMTypeRef type = NULL;

    if (type) return type;

    LLVMTypeRef fields[] = {cg->i32Type, cg->i64Type};

    type = LLVMStructCreateNamed(cg->context, "LotusValue");

    LLVMStructSetBody(type, fields, 2, false);

    return type;
}

LLVMTypeRef convertType(Type *type) {
    if (isMemoryType(type)) return LLVMPointerType(cg->i8Type, 0);
    switch (type->kind) {
        case TYPE_I32: return cg->i32Type;
        case TYPE_I64: return cg->i64Type;
        case TYPE_BOOL: return cg->boolType;
        case TYPE_F32: return cg->f32Type;
        case TYPE_F64: return cg->f64Type;
        case TYPE_STRING: return LLVMPointerType(cg->i8Type, 0);
        case TYPE_VOID: return cg->voidType;
        case TYPE_ANY: return LLVMPointerType(lotusValueLLVMType(), 0);
    }
    return cg->voidType;
}

void prepareNativeFunction(const LotusFunctionInfo *info) {
    LLVMNativeFunction *native = &cg->natives[cg->nativeCount++];

    strcpy(native->name, info->name);

    native->returnType = fromNativeType(info->returnType);
    native->argc = info->argc;
    native->variadic = info->variadic;
    native->fn = info->fn;
    LLVMAddSymbol(info->name, info->fn);

    for (int i = 0; i < info->argc; i++) native->args[i] = fromNativeType(info->args[i]);

    LLVMTypeRef params[32];
    if (info->variadic) params[0] = cg->i32Type;
    for (int i = 0; i < info->argc; i++) params[info->variadic ? i + 1 : i] = convertType(native->args[i]);

    LLVMTypeRef fnType = LLVMFunctionType(convertType(native->returnType), params, info->argc, info->variadic);
    native->fnType = fnType;

    native->llvmFunction = LLVMAddFunction(cg->module, native->name, fnType);

    typeCheckerAddNativeFunction(info->name, native->returnType, native->args, info->argc, info->variadic);
}
