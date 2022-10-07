#pragma once

// Resolves Implicitly Typed Variables into a statically typed form
// this operation is crucial to the language
// This should be done after varname mangling to ensure for consistent
// type identification

#include "../processor.h"

void resolveTypes(CompilationUnit& unit){
	for(auto member:unit.items){
		if(!instanceof<Module>(member))continue;
		auto mod = (Module*)member;
		for(auto child:mod->items){
			if(!instanceof<Method>(child))continue;
			auto source = (Method*)child;
			auto flat = source->flatten();
			for(auto child:flat){
				if(!instanceof<InitializeVar>(child))continue;
				auto init = (InitializeVar*)child;
				if(!instanceof<Primitive>(init->type))continue;
				auto type = (Primitive*)init->type;
				if(type->type != PR_implicit)continue;
				// Locate the initializer expression for this variable
				// and set its type to be equal to that of the
				// expression
				Expression* initializer = nullptr;
				for(auto c:flat){
					if(!instanceof<BinaryOperation>(c))continue;
					auto op = (BinaryOperation*)c;
					if(op->op == BIN_assignment){
						auto name = (Identifier*)op->left;
						if(name->str() == init->varname->str()){
							//TODO: Do Some Complex Initializer
							// Compatibility checks to ensure
							// consistent type safety
							initializer = op->right;

						}
					}
					
				}
				if(initializer==nullptr)error("Failed to locate type for " + init->varname->str());
				auto ctx = init->ctx;
				Logger::debug("Has " + std::to_string(ctx->items.size()) + " peers");
				LocalTypeTable type_table = ctx->local_types;
				Logger::debug("Setting Local Type");

				

				
				
			}
				
		}
	}
}


