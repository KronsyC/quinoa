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
		if(method->content)
		for(auto node : method->content->content){
			if(auto assign = dynamic_cast<InitializeVar*>(node.ptr)){
				if(assign->type){
					assign->scope->set_type(assign->var_name.str(), assign->type);
				}	
			}
			
		}
	}

	return {isGood, resolveCount};
}
