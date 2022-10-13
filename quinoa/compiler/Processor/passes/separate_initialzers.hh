//
// Separate Initializers into an initializer and an assignment
//
#pragma once
#include "../processor.h"

void split_initializers(CompilationUnit &unit)
{
    for (auto method : unit.getAllMethods())
    {
        auto flat = method->flatten();
        for (auto stm : flat)
        {
            if (instanceof <InitializeVar>(stm))
            {
                auto init = (InitializeVar *)stm;

                if (init->initializer)
                {
                    // separate the init
                    // disable the init
                    // push the new nodes to the context
                    auto newInit = new InitializeVar(init->type, init->varname);
                    init->active = false;
                    newInit->ctx = init->ctx;
                    Logger::debug(newInit->str());
                    auto newAss = new BinaryOperation(init->varname, init->initializer, BIN_assignment);
                    newAss->ctx = init->ctx;
                    pushf(*init->ctx, (Statement*)newAss);
                    pushf(*init->ctx, (Statement*)newInit);
                
                }

                
            }
        }
    }
}