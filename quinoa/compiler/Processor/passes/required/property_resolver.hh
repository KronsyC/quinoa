#pragma once

#include "../include.h"
/**
 * 
 * Inject the type definitions of all properties into the local typetable
 * 
*/


void inject_prop_type_defs(CompilationUnit& unit){
    for(auto method:unit.getAllMethods()){
        for(auto prop:unit.getAllProperties()){
            if(prop->instance_access)continue;
            (*method->local_types)[prop->name->str()] = prop->type;

            // if they are within the same module, also inject the non-prefixed equivalent
            if(prop->name->mod->refersTo && (prop->name->mod->refersTo == method->sig->name->mod->refersTo)){
                (*method->local_types)[prop->name->member->str()] = prop->type;                
            }
        }
    }
}


void resolve_props(CompilationUnit& unit){
    inject_prop_type_defs(unit);

}


