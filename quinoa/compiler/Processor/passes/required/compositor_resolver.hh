/**
 * Resolve Compositor Symbolic Module References
*/

#pragma once
#include "../include.h"


// These are the compiler voodoo modules, they have no
// real implementation
static std::vector<std::string> NATIVE_MODULES = {
    "Exported",
    "Entry",
    "CompilerImplemented"
};

void resolve_compositor_refs(Module* mod, CompilationUnit unit){
    for(auto comp:mod->compositors){
        auto name = comp->name->str();
        if(includes(NATIVE_MODULES, name))continue;
        Logger::debug("Resolving Compositor " + name);

        for(auto mod:unit.getAllModules()){
            CompoundIdentifier* fullname = new CompoundIdentifier();
            if(mod->nspace)fullname->push_back(mod->nspace);
            fullname->push_back(mod->name);
            if(name == fullname->str()){
                comp->refersTo = mod;
                break;
            }
        }
    }
}

void impl_compostors(Module* mod, CompilationUnit unit){
    for(auto c:mod->compositors){
        if(!c->refersTo)continue;
        // auto copy_me = c->refersTo;
    }
}

void resolve_compositors(CompilationUnit& unit){
    for(auto mod:unit.getAllModules()){
        resolve_compositor_refs(mod, unit);
        impl_compostors(mod, unit);
    }
}