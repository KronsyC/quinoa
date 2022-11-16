#pragma once
#include "../include.h"
//
// Validates all literal array instances to ensure that
// 1: All arrays have the proper type
// 2: Arrays are only used in initializers to variables
// 3: ?
//

void validate_array_literals(CompilationUnit &unit)
{
    // for (auto method : unit.get_methods())
    // {
    //     auto flat = method->flatten();

    //     for (auto stm : flat)
    //     {
    //         if (instanceof <InitializeVar>(stm))
    //         {
    //             auto init = (InitializeVar *)stm;
    //             if (!init->initializer)
    //                 continue;
    //             if (auto type = init->type->list())
    //             {
    //                 if (! instanceof <List>(init->initializer))
    //                     continue;
    //                 auto list = (List *)init->initializer;
    //                 list->setElementsType(type->elements);

    //                 // Make sure the list length <= Variable Length
    //                 if (type->isStatic())
    //                 {
    //                     auto varsize = ((Integer *)type->size)->value;
    //                     auto litsize = list->size();
    //                     if (litsize > varsize)
    //                         error("Array literal larger than container (" + init->str() + ")");
    //                 }
    //             }
    //         }
    //     }
    // }
}