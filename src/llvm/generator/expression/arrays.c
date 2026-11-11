#include "native_caller.h"
#include <stddef.h>
#include <llvm-c/Types.h>
#include "../../lotus_types.h"
#include "../../../parser/parser.h"
#include "arrays.h"
#include "expression_generator.h"
#include "native_functions.h"
#include "../../llvm.h"
#include <llvm-c/Core.h>

LLVMValueRef generateArray(const ArrayNode *node) {
    LotusType elementType = fromArrayType(node->base.dataType);
    LLVMValueRef size = node->size ? generateExpression(node->size) : LLVMConstInt(cg->i32Type, node->itemsCount, 0);
    LLVMValueRef array = callArrayCreate(size, elementType);
    for (size_t i = 0; i < node->itemsCount; i++) {
        LLVMValueRef value = generateExpression(node->items[i]);
        LLVMValueRef address = callArrayAt(array, LLVMConstInt(cg->i32Type, i, 0));
        LLVMBuildStore(cg->builder, value, address);
    }
    return array;
}

LLVMValueRef generateIndex(const IndexNode *node) {
    LLVMValueRef array = generateExpression(node->array);
    LLVMValueRef index = generateExpression(node->index);
    LLVMValueRef address = callArrayAt(array, index);
    LotusType elementType = node->base.dataType;
    LLVMTypeRef llvmElementType = convertType(elementType);
    LLVMValueRef value;
    if (node->value) {
        value = generateExpression(node->value);
        if (isMemoryType(elementType)) {
            LLVMValueRef old = LLVMBuildLoad2(cg->builder, llvmElementType, address, "array.old");
            releaseIfNeeded(elementType, old);
        }
        retainIfNeeded(elementType, value);
        LLVMBuildStore(cg->builder, value, address);
    } else {
        value = LLVMBuildLoad2(cg->builder, llvmElementType, address, "array.item");
        retainIfNeeded(elementType, value);
    }
    releaseIfNeeded(node->array->dataType, array);
    return value;
}

LLVMValueRef generateLength(const LengthNode *node) {
    LLVMValueRef array = generateExpression(node->array);
    LLVMValueRef length = callArrayLength(array);
    releaseIfNeeded(node->array->dataType, array);
    return length;
}
