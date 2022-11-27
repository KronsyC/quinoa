#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
class Type;
class BinaryOperation;
class LLVMValue;
class Variable
{
public:
    std::shared_ptr<Type> type;
    llvm::AllocaInst* value;
    bool constant = false;
    bool is_initialized = false;
    BinaryOperation* initializer_node = nullptr;


    Variable() = default;
    Variable(std::shared_ptr<Type> type, llvm::AllocaInst* value, bool _const = true)
    {
        this->type = type;
        this->value = value;
        this->constant = _const;
    }

    LLVMValue as_value();
};


class LLVMType;

#define VariableTable std::map<std::string, Variable>

struct ControlFlowInfo
{
    // The block to jump to after the `break` action is invoked
    llvm::BasicBlock *breakTo;

    // The block to jump to after the `continue` action is invoked
    llvm::BasicBlock *continueTo;

    // The block to jump to after the `fallthrough` action is invoked
    llvm::BasicBlock *fallthroughTo;

    // The block to break to after the inner scope is executed
    llvm::BasicBlock *exitBlock;
};

llvm::LLVMContext* llctx();
llvm::IRBuilder<>* builder();

LLVMValue cast(LLVMValue val, LLVMType type);

LLVMType getCommonType(LLVMType t1, LLVMType t2);

bool isInt(LLVMType t);
