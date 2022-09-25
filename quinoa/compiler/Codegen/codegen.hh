#pragma once
#include<llvm/IR/Module.h>
#include "../AST/ast.hh"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
static llvm::LLVMContext ctx;
static llvm::IRBuilder<> builder(ctx);

namespace Codegen{
    llvm::Module* codegen(CompilationUnit& ast);
};