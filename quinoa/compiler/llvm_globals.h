#pragma once


#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
struct Type;
class Variable{
public:
    Type* type;
    llvm::AllocaInst* value;
    bool constant = false;

    Variable(Type* type, llvm::AllocaInst* value, bool _const = true){
        this->type = type;
        this->value = value;
        this->constant = _const;
    }
};

#define TVars std::map<std::string, Variable*>

llvm::LLVMContext* llctx();
llvm::IRBuilder<>* builder();

llvm::Value *cast(llvm::Value *val, llvm::Type *type);


llvm::Type *getCommonType(llvm::Type *t1, llvm::Type *t2);

bool isInt(llvm::Type *t);

