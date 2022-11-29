#pragma once

#include "../include.h"
#include "../../../AST/literal.hh"
//
// Validates all literal array instances to ensure that
// 1: Arrays are only used as variable initializers
// 2: All assigned arrays must be equal in length to their variable
//

void validate_array_literals(CompilationUnit &unit) {
   for(auto method : unit.get_methods()){
       if(!method->content)continue;
        for(auto code : method->content->flatten()){
            if(auto literal = dynamic_cast<ArrayLiteral*>(code)){

                auto parent_node = literal->scope->parent_of(literal);
                if(!parent_node)except(E_INTERNAL, "List literal without parent node: " + literal->str());

                auto initializer = dynamic_cast<InitializeVar*>(parent_node);

                if(!initializer)except(E_BAD_ASSIGNMENT, "An array literal may only be used as an operand of an initialization statement");

                auto initializer_ty = initializer->type;

                auto arr_ty = initializer_ty->get<ListType>();

                if(!arr_ty)except(E_BAD_ASSIGNMENT, "Array literals may only be assigned to a variable with an array type");

                auto my_len = literal->members.len();
                auto arr_len = arr_ty->size->value;

                if(my_len < arr_len)
                    except(E_BAD_ARRAY_LEN,
                           "An array literal must be the same size as its variable, but was found to be larger\n" \
                           "\t\t Literal length: " + std::to_string(my_len) + "; Variable Length: " + std::to_string(arr_len)

                );
                if(my_len > arr_len)
                    except(E_BAD_ARRAY_LEN,
                           "An array literal must be the same size as its variable, but was found to be smaller\n" \
                           "\t\t Literal length: " + std::to_string(my_len) + "; Variable Length: " + std::to_string(arr_len)

                );
            }
        }
   }
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