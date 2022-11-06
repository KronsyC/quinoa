//
// Separate Initializers into an initializer and an assignment
//
#pragma once
#include "../include.h"

void split_initializers(CompilationUnit &unit)
{
    // for (auto method : unit.getAllMethods())
    // {
    //     auto flat = method->flatten();
    //     for (auto stm : flat)
    //     {
    //         if (instanceof<InitializeVar>(stm))
    //         {
    //             auto init = (InitializeVar *)stm;

    //             if (init->initializer)
    //             {
    //                 // separate the init
    //                 // push the new node to the context, after the init
    //                 auto assignment = new BinaryOperation(init->varname, init->initializer, BIN_assignment);
    //                 assignment->ctx = init->ctx;
    //                 assignment->manufactured = true;
    //                 init->initializer = nullptr;
    //                 int idx = indexof(*init->ctx, (Statement*)init);
    //                 if(idx==-1)error("Failed to get index of " + init->str());
    //                 // push the assignment into the context
    //                 init->ctx->insert(init->ctx->begin()+idx+1, assignment);
    //             }

                
    //         }
    //     }
    // }
}