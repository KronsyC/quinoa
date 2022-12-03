#pragma once

#include "./include.h"


#include "./finalize/entrypoint_resolver.hh"
#include "./finalize/separate_initialzers.hh"
#include "./required/type_normalizer.hh"
void finalize_ast(CompilationUnit &ast) {
    split_initializers(ast);
    normalize_types(ast);
    gen_entrypoint(ast);
}