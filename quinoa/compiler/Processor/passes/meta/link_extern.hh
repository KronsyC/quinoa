#pragma once
#include "../include.h"
/*
 * 
 * The link_extern method metadata specifies that a method is to be declared with
 * the specified signature, but implementation is externally provided, a good example
 * of this is the glibc dependency
 * 
 */



void link_extern(CompilationUnit& unit){
    for(auto fn:unit.getAllMethods()){
        auto meta = fn->metadata;
        for(auto tag:meta){
            if(tag->name!="link_extern")continue;
            if(fn->size())error("link_extern signature must have no content");
            if(tag->parameters.size() != 1)error("link_extern tag expects one string argument");
            auto name = (String*)tag->parameters[0];
            if(!instanceof<String>(name))error("link_extern expects a string");
            fn->sig->name = Ident::get(name->value);
            fn->sig->nomangle = true;
            Logger::debug("Generate link_extern intrinsic for " + name->value);
        }
    }
}