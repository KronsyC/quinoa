#pragma once
#include<llvm/IR/Module.h>
#include "../AST/ast.hh"

#include "../llvm_globals.h"

namespace Codegen{
    llvm::Module* codegen(CompilationUnit& ast);
};