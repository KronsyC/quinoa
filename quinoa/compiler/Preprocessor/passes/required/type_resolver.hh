#pragma once

// Resolves Implicitly Typed Variables into a statically typed form
// this operation is crucial to the language
// This should be done after varname mangling to ensure for consistent
// type identification

#include "../include.h"

// Returns True, if all types are resolved
std::pair<bool, int> resolve_types(CompilationUnit &unit)
{
	bool isGood = true;
	int resolveCount = 0;

	for(auto method : unit.get_methods()){
		if(!method->content)continue;

		for(auto prop:unit.get_properties()){
			method->content->set_type(prop->name->str(), prop->type);

			if(prop->name->container == method->name->container){
				method->content->set_type(prop->name->member->str(), prop->type);
			}
		}

		for(auto param : method->parameters){
			method->content->set_type(param->name.str(), param->type);
		}
		
		for(auto node : method->content->flatten()){
			if(auto assign = dynamic_cast<InitializeVar*>(node)){
				if(assign->type){
					assign->scope->set_type(assign->var_name.str(), assign->type);
				}	
			}
			
		}
	}

	return {isGood, resolveCount};
}
