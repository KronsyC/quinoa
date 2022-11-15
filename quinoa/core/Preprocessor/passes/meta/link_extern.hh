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
    for(auto fn:unit.get_methods()){
        auto& meta = fn->attrs;
        for(auto tag:meta){
            if(tag->name!="link_extern")continue;
            if(fn->content)except(E_BAD_METADATA, "link_extern method must be a signature");
            if(tag->arguments.len() != 1)error("link_extern tag expects one string argument");
            auto& name = tag->arguments[0];
            if(!dynamic_cast<String*>(&name))error("link_extern expects a string");
            fn->name->trunc = true;
            fn->name->member->set_name(name.str().substr(1, name.str().size()-2));

        }
    }
}