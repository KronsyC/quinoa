#pragma once
#include "../AST_old/ast.hh"
namespace Processor
{
void process(CompilationUnit* unit, bool finalize = true);
}