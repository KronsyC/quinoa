#pragma once
#include "../AST/ast.hh"
namespace Processor{
    void process(CompilationUnit& unit, bool finalize=true);
}