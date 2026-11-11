#include "../../utils/llvm_extension.h"
#include "native_caller.h"
#include "native_functions.h"
#include "expression_generator.h"
#include "../../llvm.h"
#include "../../../utils/cutils.h"
#include <string.h>
#include "../../lotus_types.h"
#include "../../../parser/parser.h"
#include <llvm-c/Types.h>
#include <llvm-c/Core.h>

static LLVMNativeFunction *findNative(const char *name) {
    for (int i = 0; i < cg->nativeCount; i++) {
        if (strcmp(cg->natives[i].name, name) == 0) return &cg->natives[i];
    }

    exitWithError("Native function %s is unavailable; load a compatible runtime library", name);
}

static LLVMValueRef nativeCall(const char *name, LLVMValueRef *args, unsigned count) {
    LLVMNativeFunction *fn = findNative(name);
    return LLVMBuildCall2(cg->builder, fn->fnType, fn->llvmFunction, args, count, "");
}

LLVMValueRef callArrayCreate(LLVMValueRef size, LotusType elementType) {
    LLVMValueRef args[] = {size, LLVMConstInt(cg->i32Type, elementType, 0)};
    return nativeCall("_arrayCreate", args, 2);
}

LLVMValueRef callArrayAt(LLVMValueRef array, LLVMValueRef index) {
    LLVMValueRef args[] = {array, index};
    return nativeCall("_arrayAt", args, 2);
}

LLVMValueRef callArrayLength(LLVMValueRef array) {
    return nativeCall("_arrayLength", &array, 1);
}

LLVMValueRef callStringEquals(LLVMValueRef left, LLVMValueRef right) {
    LLVMValueRef args[] = {left, right};
    return nativeCall("_stringEquals", args, 2);
}

LLVMValueRef callStringConcat(LLVMValueRef left, LLVMValueRef right) {
    LLVMValueRef args[] = {left, right};
    return nativeCall("_concat", args, 2);
}

void retainIfNeeded(LotusType type, LLVMValueRef value) {
    if (type == LOTUS_STRING) nativeCall("_stringRetain", &value, 1);
    else if (isArrayType(type)) nativeCall("_arrayRetain", &value, 1);
}

void releaseIfNeeded(LotusType type, LLVMValueRef value) {
    if (type == LOTUS_STRING) nativeCall("_stringRelease", &value, 1);
    else if (isArrayType(type)) nativeCall("_arrayRelease", &value, 1);
}

static LLVMValueRef buildLotusValue(LotusType type, LLVMValueRef value) {
    LLVMTypeRef valueType = lotusValueLLVMType();
    LLVMValueRef result = LLVMGetUndef(valueType);

    result = LLVMBuildInsertValue(cg->builder, result, LLVMConstInt(cg->i32Type, type, 0), 0, "");

    LLVMValueRef payload = NULL;

    switch (type) {
        case LOTUS_I32: payload = LLVMBuildSExt(cg->builder, value, cg->i64Type, ""); break;

        case LOTUS_I64: payload = value; break;

        case LOTUS_BOOL: payload = LLVMBuildZExt(cg->builder, value, cg->i64Type, ""); break;

        case LOTUS_STRING: payload = LLVMBuildPtrToInt(cg->builder, value, cg->i64Type, ""); break;

        case LOTUS_F32:
            payload = LLVMBuildFPExt(cg->builder, value, cg->f64Type, "");

            payload = LLVMBuildBitCast(cg->builder, payload, cg->i64Type, "");
            break;

        case LOTUS_F64: payload = LLVMBuildBitCast(cg->builder, value, cg->i64Type, ""); break;

        default: payload = LLVMConstInt(cg->i64Type, 0, 0);
    }

    result = LLVMBuildInsertValue(cg->builder, result, payload, 1, "");

    LLVMValueRef storage = createEntryAlloca(valueType, "boxed.argument");
    LLVMBuildStore(cg->builder, result, storage);
    return storage;
}

LLVMValueRef generateNativeCall(const CallNode *node) {
    LLVMNativeFunction *native = findNative(node->callee->name);
    LLVMValueRef values[MAX_ARGS];
    LLVMValueRef args[MAX_ARGS + 1];
    unsigned count = 0;

    // Variadic native functions receive the argument count before their values
    if (native->variadic) args[count++] = LLVMConstInt(cg->i32Type, node->argumentCount, 0);
    for (int i = 0; i < node->argumentCount; i++) {
        const Node *arg = node->arguments[i];
        int param = i < native->argc ? i : native->argc - 1;
        values[i] = generateExpression(arg);
        args[count++] = native->args[param] == LOTUS_ANY ? buildLotusValue(arg->dataType, values[i]) : values[i];
    }

    LLVMValueRef result = LLVMBuildCall2(cg->builder, native->fnType, native->llvmFunction, args, count, "");
    for (int i = 0; i < node->argumentCount; i++) releaseIfNeeded(node->arguments[i]->dataType, values[i]);
    return result;
}
