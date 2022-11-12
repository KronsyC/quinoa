#pragma once
#include "../AST/compilation_unit.hh"
#include "../../lib/logger.h"
#include "../../lib/error.h"

#include "./passes/required_processes.hh"
#include "./passes/finalization.hh"
#include "./passes/syntactic_sugar.hh"
#include "./passes/metadata.hh"
namespace Preprocessor
{
    void process_ast(CompilationUnit& unit, bool finalize){

        // except(E_INTERNAL, "Preprocessor not implemented");
        apply_syntactic_sugar(unit);
        process_required(&unit);
        if(finalize){
            process_metadata(unit);
            finalize_ast(unit);
        }
    }
};
