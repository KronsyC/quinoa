#pragma once
#include "../AST/ast.hh"
#include <llvm/IR/Module.h>

#include "../llvm_globals.h"

namespace Codegen
{
llvm::Module* codegen(CompilationUnit& ast);
};