#pragma once


#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#define TVars std::map<std::string, llvm::AllocaInst *>
static llvm::LLVMContext ctx;
static llvm::IRBuilder<> builder(ctx);

// bool isInt(llvm::Type *i)
// {
//     return i->isIntegerTy(1) || i->isIntegerTy(8) || i->isIntegerTy(16) || i->isIntegerTy(32) || i->isIntegerTy(64);
// }
// bool isFloat(llvm::Type *i)
// {
//     return i->isFloatingPointTy() || i->isHalfTy() || i->isDoubleTy();
// }


