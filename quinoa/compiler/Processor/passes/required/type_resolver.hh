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
	for (auto source : unit.getAllMethods())
	{
		auto flat = source->flatten();
		// Resolve Module-as-type references
		for (auto child : flat)
		{

			Logger::debug("Resolve type");
			if (instanceof <ModuleRef>(child))
			{
				Logger::debug("ModRef");
			}
		}

		// Resolve Untyped Variables
		for (auto child : flat)
		{
			if (! instanceof <InitializeVar>(child))
				continue;
			auto init = static_cast<InitializeVar *>(child);

			if (init->type)
				continue;
			// Locate the initializer expression for this variable
			// and set its type to be equal to that of the
			// expression
			Expression *initializer = init->initializer;
			for (auto c : flat)
			{
				if (! instanceof <BinaryOperation>(c))
					continue;
				auto op = (BinaryOperation *)c;
				if (op->op != BIN_assignment)
					continue;
				if (! instanceof <Identifier>(op->left))
					continue;

				auto name = static_cast<Identifier *>(op->left);
				if (name->str() == init->varname->str())
				{
					// TODO: Do Some Complex Initializer
					//  Compatibility checks to ensure
					//  consistent type safety
					initializer = op->right;
				}
			}
			if (!initializer)
				error("Failed to locate type for " + init->varname->str());
			auto ctx = initializer->ctx;
			if (!ctx)
				Logger::error("No Context found for initializer expression");
			if (!ctx->local_types)
				error("Local Type Table is null", false);
			auto exprType = initializer->getType();
			// If a nullptr is returned, there is not enough info
			// currently available
			if (exprType == nullptr)
			{
				isGood = false;
				continue;
			}
			(*init->ctx->local_types)[init->varname->str()] = exprType;
			init->type = exprType;
			resolveCount++;
		}
	}
	return {isGood, resolveCount};
}
