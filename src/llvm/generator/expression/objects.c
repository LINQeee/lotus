#include "objects.h"

#include <llvm-c/Core.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <llvm-c/Types.h>

#include "expression_generator.h"
#include "native_functions.h"
#include "native_caller.h"
#include "functions.h"
#include "../../llvm.h"
#include "../../utils/llvm_extension.h"
#include "../../../utils/cutils.h"
#include "../../../lexer/lexer.h"
#include "../../../parser/objects.h"
#include "../../../parser/parser.h"
#include "../../types.h"

typedef struct {
    ObjectType *type;
    LLVMTypeRef layout;
    LLVMValueRef retain;
    LLVMValueRef release;
} ObjectCodegen;
static ObjectCodegen *objects;
static LLVMTypeRef lifetimeType;

static ObjectCodegen *findObjectCodegen(Type *type) {
    for (size_t i = 0; i < objectTypeCount; i++)
        if (objects[i].type == type->object) return &objects[i];
    exitWithError("Object type was not prepared: %s", typeName(type));
}

void retainObject(Type *type, LLVMValueRef value) {
    LLVMBuildCall2(cg->builder, lifetimeType, findObjectCodegen(type)->retain, &value, 1, "");
}
void releaseObject(Type *type, LLVMValueRef value) {
    LLVMBuildCall2(cg->builder, lifetimeType, findObjectCodegen(type)->release, &value, 1, "");
}
void prepareObjectCodegen() {
    objects = mallocSafe((objectTypeCount ? objectTypeCount : 1) * sizeof(ObjectCodegen));
    LLVMTypeRef pointer = LLVMPointerType(cg->i8Type, 0);
    lifetimeType = LLVMFunctionType(cg->voidType, &pointer, 1, false);
    for (size_t i = 0; i < objectTypeCount; i++) {
        ObjectType *type = objectTypes[i];
        ObjectCodegen *object = &objects[i];
        object->type = type;
        object->layout = LLVMStructCreateNamed(cg->context, type->name);
        LLVMTypeRef *fields = mallocSafe((type->fieldCount + 1) * sizeof(LLVMTypeRef));
        fields[0] = cg->i64Type;
        for (size_t j = 0; j < type->fieldCount; j++) fields[j + 1] = convertType(type->fields[j].type);
        LLVMStructSetBody(object->layout, fields, type->fieldCount + 1, false);
        free(fields);
        char name[80];
        snprintf(name, sizeof(name), "$object.retain.%zu", i);
        object->retain = LLVMAddFunction(cg->module, name, lifetimeType);
        LLVMSetLinkage(object->retain, LLVMInternalLinkage);
        snprintf(name, sizeof(name), "$object.release.%zu", i);
        object->release = LLVMAddFunction(cg->module, name, lifetimeType);
        LLVMSetLinkage(object->release, LLVMInternalLinkage);
    }
    for (size_t i = 0; i < objectTypeCount; i++) {
        ObjectType *type = objectTypes[i];
        ObjectCodegen *object = &objects[i];
        for (int release = 0; release < 2; release++) {
            LLVMValueRef fn = release ? object->release : object->retain;
            LLVMValueRef value = LLVMGetParam(fn, 0);
            LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(cg->context, fn, "entry");
            LLVMBasicBlockRef update = LLVMAppendBasicBlockInContext(cg->context, fn, "update");
            LLVMBasicBlockRef done = LLVMAppendBasicBlockInContext(cg->context, fn, "done");
            LLVMPositionBuilderAtEnd(cg->builder, entry);
            LLVMBuildCondBr(cg->builder, LLVMBuildIsNull(cg->builder, value, "is.null"), done, update);
            LLVMPositionBuilderAtEnd(cg->builder, update);
            LLVMValueRef address = LLVMBuildStructGEP2(cg->builder, object->layout, value, 0, "references");
            LLVMValueRef count = LLVMBuildLoad2(cg->builder, cg->i64Type, address, "count");
            LLVMValueRef one = LLVMConstInt(cg->i64Type, 1, false);
            count = release ? LLVMBuildSub(cg->builder, count, one, "remaining")
                            : LLVMBuildAdd(cg->builder, count, one, "retained");
            LLVMBuildStore(cg->builder, count, address);
            if (release) {
                LLVMBasicBlockRef destroy = LLVMAppendBasicBlockInContext(cg->context, fn, "destroy");
                LLVMBuildCondBr(cg->builder,
                                LLVMBuildICmp(cg->builder, LLVMIntEQ, count, LLVMConstNull(cg->i64Type), "last"),
                                destroy, done);
                LLVMPositionBuilderAtEnd(cg->builder, destroy);
                for (size_t j = 0; j < type->fieldCount; j++) {
                    Type *fieldType = type->fields[j].type;
                    if (!isMemoryType(fieldType)) continue;
                    LLVMValueRef field = LLVMBuildStructGEP2(cg->builder, object->layout, value, j + 1, "field");
                    releaseIfNeeded(fieldType, LLVMBuildLoad2(cg->builder, convertType(fieldType), field, "value"));
                }
                LLVMBuildFree(cg->builder, value);
            }
            LLVMBuildBr(cg->builder, done);
            LLVMPositionBuilderAtEnd(cg->builder, done);
            LLVMBuildRetVoid(cg->builder);
        }
    }
}
LLVMValueRef generateObject(const ObjectNode *node) {
    ObjectType *type = getObjectType(node->base.dataType);
    ObjectCodegen *object = findObjectCodegen(node->base.dataType);
    LLVMValueRef *values = mallocSafe((type->fieldCount ? type->fieldCount : 1) * sizeof(LLVMValueRef));
    for (size_t i = 0; i < type->fieldCount; i++) values[i] = NULL;
    for (size_t i = 0; i < node->count; i++) values[node->indices[i]] = generateExpression(node->values[i]);
    for (size_t i = 0; i < type->fieldCount; i++) {
        if (values[i]) continue;
        LLVMUserFunction *fn = findFunction(type->fields[i].initializer->name);
        values[i] = LLVMBuildCall2(cg->builder, fn->fnType, fn->llvmFunction, NULL, 0, "default.value");
    }
    LLVMValueRef value = LLVMBuildMalloc(cg->builder, object->layout, "object");
    LLVMBasicBlockRef failed = LLVMAppendBlock("object.allocation.failed");
    LLVMBasicBlockRef ready = LLVMAppendBlock("object.ready");
    LLVMBuildCondBr(cg->builder, LLVMBuildIsNull(cg->builder, value, "allocation.failed"), failed, ready);
    LLVMMoveBuilderToEnd(failed);
    unsigned trapId = LLVMLookupIntrinsicID("llvm.trap", 9);
    LLVMValueRef trap = LLVMGetIntrinsicDeclaration(cg->module, trapId, NULL, 0);
    LLVMBuildCall2(cg->builder, LLVMGlobalGetValueType(trap), trap, NULL, 0, "");
    LLVMBuildUnreachable(cg->builder);
    LLVMMoveBuilderToEnd(ready);
    LLVMValueRef references = LLVMBuildStructGEP2(cg->builder, object->layout, value, 0, "references");
    LLVMBuildStore(cg->builder, LLVMConstInt(cg->i64Type, 1, false), references);
    for (size_t i = 0; i < type->fieldCount; i++) {
        LLVMValueRef address = LLVMBuildStructGEP2(cg->builder, object->layout, value, i + 1, "field");
        LLVMBuildStore(cg->builder, values[i], address);
    }
    free(values);
    return value;
}
LLVMValueRef generateMember(const MemberNode *node) {
    LLVMValueRef object = generateExpression(node->object);
    if (isArrayType(node->object->dataType)) {
        LLVMValueRef length = callArrayLength(object);
        releaseIfNeeded(node->object->dataType, object);
        return length;
    }
    LLVMTypeRef layout = findObjectCodegen(node->object->dataType)->layout;
    LLVMValueRef address = LLVMBuildStructGEP2(cg->builder, layout, object, node->index + 1, "field.address");
    LLVMTypeRef type = convertType(node->base.dataType);
    LLVMValueRef value;
    if (node->value) {
        LLVMValueRef old = NULL;
        if (node->op != TOKEN_EQUAL) {
            old = LLVMBuildLoad2(cg->builder, type, address, "field.old");
            retainIfNeeded(node->base.dataType, old);
        }
        value = generateExpression(node->value);
        if (node->op != TOKEN_EQUAL) {
            TokenKind op;
            switch (node->op) {
                case TOKEN_EQ_PLUS: op = TOKEN_PLUS; break;
                case TOKEN_EQ_MINUS: op = TOKEN_MINUS; break;
                case TOKEN_EQ_STAR: op = TOKEN_STAR; break;
                case TOKEN_EQ_SLASH: op = TOKEN_SLASH; break;
                default: op = TOKEN_PERCENT; break;
            }
            Node operand = {.dataType = node->base.dataType};
            BinaryNode binary = {.base = {.dataType = node->base.dataType}, .op = op, .left = &operand};
            value = generateBinaryValues(&binary, old, value);
        }
        LLVMValueRef previous = LLVMBuildLoad2(cg->builder, type, address, "field.previous");
        releaseIfNeeded(node->base.dataType, previous);
        retainIfNeeded(node->base.dataType, value);
        LLVMBuildStore(cg->builder, value, address);
    } else {
        value = LLVMBuildLoad2(cg->builder, type, address, "field.value");
        retainIfNeeded(node->base.dataType, value);
    }
    releaseIfNeeded(node->object->dataType, object);
    return value;
}
