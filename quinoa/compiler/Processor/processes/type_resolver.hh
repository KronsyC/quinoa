#pragma once

// Resolves Implicitly Typed Variables into a statically typed form
// this operation is crucial to the language
// This should be done after varname mangling to ensure for consistent
// type identification

#include "../processor.h"

// Returns True, if all types are resolved
bool resolveTypes(CompilationUnit& unit){
	bool isGood = true;
	for(auto source:unit.getAllMethods()){
		auto flat = source->flatten();
		for(auto child:flat){
			if(!instanceof<InitializeVar>(child))continue;
			auto init = static_cast<InitializeVar*>(child);

			if(init->type != nullptr)continue;
			// Locate the initializer expression for this variable
			// and set its type to be equal to that of the
			// expression
			Expression* initializer = nullptr;
			for(auto c:flat){
				if(!instanceof<BinaryOperation>(c))continue;
				auto op = (BinaryOperation*)c;
				if(op->op != BIN_assignment)continue;

				auto name = static_cast<Identifier*>(op->left);
				if(name->str() == init->varname->str()){
					//TODO: Do Some Complex Initializer
					// Compatibility checks to ensure
					// consistent type safety
					initializer = op->right;
				}	
			}
			if(initializer==nullptr)error("Failed to locate type for " + init->varname->str());
			auto ctx = initializer->ctx;
			if(ctx==nullptr)ctx=init->ctx;
			if(ctx==nullptr)error("No ctx was found for init");
			if(ctx->local_types==nullptr)error("Local Type Table is null", false);
			auto exprType = initializer->getType();
			// If a nullptr is returned, there is not enough info
			// currently available
			if(exprType == nullptr){
				isGood = false;
				continue;
			}
			(*init->ctx->local_types)[init->varname->str()] = exprType;
			init->type = exprType;
	}
	}
	return isGood;
}


