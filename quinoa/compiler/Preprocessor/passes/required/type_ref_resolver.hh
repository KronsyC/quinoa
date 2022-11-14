#pragma once

/**
* All Type References can be resolved immediately
 * i.e they never depend on inferrable data
*/
#include "../include.h"

void attempt_resolve_typeref(TypeRef& ref, Method* method){
    Logger::debug("Resolving ref: " + ref.name->str());
    // Check the local container type definitions
    auto locally_defined_type = method->parent->get_type(ref.name->str());
    if(locally_defined_type){
        ref.resolves_to = locally_defined_type;
        Logger::debug("Resolved locally as: " + ref.resolves_to->str());
        return;
    }
    except(E_UNRESOLVED_TYPE, "Failed to resolve type " + ref.name->str());
}

void resolve_type_references(CompilationUnit& unit){

    for(auto method : unit.get_methods()){
        // Resolve Return Type (if it is a type-ref)
        if(auto ref = method->return_type->drill()->get<TypeRef>()){
            attempt_resolve_typeref(*ref, method);
        }

        // Resolve Parameter Type (if it is a type-ref)
        for(auto param : method->parameters){
            if(auto ref = param->type->drill()->get<TypeRef>()){
                attempt_resolve_typeref(*ref, method);
            }
        }

    }

}