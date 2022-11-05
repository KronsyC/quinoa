#pragma once
#include "../AST/compilation_unit.hh"


namespace Parser{
    std::unique_ptr<CompilationUnit> make_ast(std::vector<Token>& toks);
};