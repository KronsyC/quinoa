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

void resolve_compositor_refs(Container* mod, CompilationUnit unit){
    for(auto& comp:mod->compositors){
        auto name = comp->name->str();
        if(includes(NATIVE_MODULES, name))continue;
        Logger::debug("Resolving Compositor " + name);

        for(auto mod:unit.get_containers()){
            auto fullname = std::make_unique<LongName>();
            if(mod->name_space)fullname->parts.push(*mod->name_space);
            fullname->parts.push(*mod->name);
            if(name == fullname->str()){
                comp->refers_to = mod;
                break;
            }
        }
        if(!comp->refers_to){
            error("Failed to resolve compositor for " + comp->name->str());
        }
    }
}


void resolve_compositors(CompilationUnit& unit){
    for(auto mod:unit.get_containers()){
        resolve_compositor_refs(mod, unit);
    }
}