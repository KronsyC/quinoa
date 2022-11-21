#pragma once
#include "../AST/ast.hh"
#include <llvm/IR/Module.h>

#include "../llvm_utils.h"
#include "../AST/compilation_unit.hh"
namespace Codegen
{
    llvm::Module* codegen(CompilationUnit& ast);
};