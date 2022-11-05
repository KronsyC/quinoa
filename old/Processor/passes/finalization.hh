#pragma once
#include "./include.h"


#include "./finalize/entrypoint_resolver.hh"
#include "./finalize/generic_implementer.hh"
#include "./finalize/member_hoister.hh"
#include "./finalize/separate_initialzers.hh"


void finalize_ast(CompilationUnit& ast){
    impl_generics(ast);
    split_initializers(ast);
    hoist_definitions(ast);
    gen_entrypoint(ast);
}