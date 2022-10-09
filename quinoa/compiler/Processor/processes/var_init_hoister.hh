#pragma once
#include "../processor.h"
// hoisting variable initializations to the very beginning makes variables declared within loops
// NOT cause memory leaks
// is a recommened frontend optimization by the llvm team

void mangleVarNames(SourceBlock &code, std::vector<std::string> &occupiedNames)
{
	for (auto member : code.items)
	{
		if (instanceof <InitializeVar>(member))
		{
			auto var = (InitializeVar *)member;
			auto name = var->varname;
			if (includes(occupiedNames, name->str()))
				error("Multiple Variables with the same name are currently not supported");
			occupiedNames.push_back(name->str());
		}
	}
}

void hoistVarInitializations(CompilationUnit &unit)
{
	// Locate all methods
	for (auto method : unit.getAllMethods())
	{
		// perform any necessary mangling
		std::vector<std::string> allVarNames;
		mangleVarNames(*method, allVarNames);
		// flatten it
		auto flat = method->flatten();
		// remove all declarations and push them into a list
		std::vector<InitializeVar *> allInitializations;
		for (int i = 0; i < flat.size(); i++)
		{
			auto item = flat[i];
			if (instanceof <InitializeVar>(item))
			{
				auto v = (InitializeVar *)item;
				allInitializations.push_back(new InitializeVar(*v));
				v->active = false;
			}
		}
		// insert all declarations into the top
		for (auto init : allInitializations)
		{
			pushf(method->items, (Statement *)init);
		}
	}
}