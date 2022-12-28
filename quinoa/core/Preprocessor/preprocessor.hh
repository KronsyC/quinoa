#pragma once

#include "../../lib/error.h"
#include "../../lib/logger.h"
#include "../AST/compilation_unit.hh"

#include "./passes/finalization.hh"
#include "./passes/metadata.hh"
#include "./passes/required_processes.hh"
#include "./passes/syntactic_sugar.hh"

namespace Preprocessor {
void process_ast(CompilationUnit& unit, bool finalize) {
    apply_syntactic_sugar(unit);
    //        for (auto fn: unit.get_methods()) {
    //            if (fn->content) {
    //                Logger::debug("\nfunc " + fn->source_name() + "(){\n" + fn->content->str() + "\n}\n");
    //            } else Logger::debug("\nfunc " + fn->source_name() + "();");
    //
    //        }
    //        except(E_INTERNAL, "early exit(test)");

    process_required(&unit);
    if (finalize) {
        process_metadata(unit);
        finalize_ast(unit);
    }
}
}; // namespace Preprocessor
