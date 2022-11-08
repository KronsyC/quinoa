//
// Separate Initializers into an initializer and an assignment
// This is useful, as a future optimization that may be made is
// hoisting all variable declarations to the top of the content
// which may cause issues if a variable is initialized with the return value 
// of a function call relying on another variable, or other such scenarios
//
#pragma once
#include "../include.h"
#include "../../../AST/symbol_operators.hh"
void split_initializers(CompilationUnit &unit)
{
    for(auto method : unit.get_methods()){
        for(auto code : method->content->flatten()){
            if(auto init = dynamic_cast<InitializeVar*>(code)){
                if(init->initializer){
                    auto assignment = std::make_unique<BinaryOperation>(std::make_unique<SourceVariable>(init->var_name), std::move(init->initializer), BIN_assignment);
                    assignment->scope = init->scope;
                    int idx = init->scope->content.indexof(init);
                    if(idx==-1)error("Failed to get index of " + init->str());
                    // push the assignment into the context
                    auto& ref = *assignment.release();
                    init->scope->content.set(idx, ref);
                    init->scope->content.pushf(*init);
                }
            }
        }
    }
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