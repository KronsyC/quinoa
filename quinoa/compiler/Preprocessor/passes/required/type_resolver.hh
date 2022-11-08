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

	return {isGood, resolveCount};
}
