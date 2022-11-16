#pragma once

/**
* All Type References can be resolved immediately
 * i.e they never depend on inferrable data
*/
#include "../include.h"
void attempt_resolve_typeref(TypeRef& ref, Container* container) {
    auto locally_defined_type = container->get_type(ref.name->str());
    if(locally_defined_type){
        ref.resolves_to = locally_defined_type;
        Logger::debug("Resolved locally as: " + ref.resolves_to->str());
        return;
    }

    // Check other modules
    for(auto type : container->parent->get_types()){
        if(type->name->str() == ref.name->str()){
            ref.resolves_to = type->refers_to;
            Logger::debug("Resolved remotely as: " + ref.resolves_to->str());
            return;
        }
    }
}
void attempt_resolve_typeref(TypeRef& ref, Method* method){
    Logger::debug("Resolving ref: " + ref.name->str());

    attempt_resolve_typeref(ref, method->parent);

    // Check the local container type definitions

    if(!ref.resolves_to)except(E_UNRESOLVED_TYPE, "Failed to resolve type " + ref.name->str());
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

        // Resolve Initializer Type Refs
        if(!method->content)continue;
        for(auto code : method->content->flatten()){
            if(auto init = dynamic_cast<InitializeVar*>(code)){
                if(!init->type)continue;
                if(auto ref = init->type->drill()->get<TypeRef>()){

                    attempt_resolve_typeref(*ref, method);
                }
            }
        }
    }

    for(auto cont : unit.get_containers()){
        for(auto mem : cont->members){
            if(auto type = dynamic_cast<TypeMember*>(mem.ptr)){

                for(auto flat : type->refers_to->flatten()){
                    if(auto ref = flat->drill()->get<TypeRef>()){
                        if(ref->resolves_to)continue;
                        Logger::debug("Resolve type ref to: " + ref->str());
                        attempt_resolve_typeref(*ref, cont);
                        Logger::debug("It is now: " + ref->str());
                    }
                }


            }
        }
    }

}