#pragma once
#include<llvm/IR/Module.h>
#include "../AST/ast.hh"

namespace Codegen{
    llvm::Module* codegen(CompilationUnit& ast);
};