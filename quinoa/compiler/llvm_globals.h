#pragma once


#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#define TVars std::map<std::string, llvm::AllocaInst *>

llvm::LLVMContext* ctx();
llvm::IRBuilder<>* builder();

