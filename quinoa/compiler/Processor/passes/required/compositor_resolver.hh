/**
 * Resolve Compositor Symbolic Module References
*/

#pragma once
#include "../include.h"


// These are the compiler voodoo modules, they have no
// real implementation
static std::vector<std::string> NATIVE_MODULES = {
    "Exported",
    "Entry"
};

void resolve_compositors(Module* mod, CompilationUnit unit){
    for(auto comp:mod->compositors){
        auto name = comp->name->str();
        if(includes(NATIVE_MODULES, name))continue;
        Logger::debug("Resolving Compositor " + name);
    }
}

void resolve_compositors(CompilationUnit& unit){
    for(auto mod:unit.getAllModules()){
        resolve_compositors(mod, unit);
    }
}