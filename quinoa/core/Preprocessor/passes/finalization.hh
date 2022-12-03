#pragma once

#include "./include.h"


#include "./finalize/entrypoint_resolver.hh"
#include "./finalize/separate_initialzers.hh"
void finalize_ast(CompilationUnit &ast) {
    split_initializers(ast);
    gen_entrypoint(ast);
}