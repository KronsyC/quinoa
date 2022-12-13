#pragma once

#include "../processor.h"
// hoisting variable initializations to the very beginning makes variables declared within loops
// NOT cause memory leaks
// is a recommened frontend optimization by the llvm team

void mangleVarNames(SourceBlock &code) {
    std::vector<std::string> occupiedNames;
    //
    std::vector< InitializeVar * > traversedInits;
    for (auto member: code) {
        if (instanceof<InitializeVar>(member)) {
            auto var = (InitializeVar *) member;
            if (!var->active)continue;
            auto name = var->varname;
            if (includes(occupiedNames, name->str()) && !includes(traversedInits, var))
                error("Multiple Variables with the same name are currently not supported");
            if (includes(traversedInits, var))continue;
            traversedInits.push_back(var);
            occupiedNames.push_back(name->str());
        }
    }
}

void hoistVarInitializations(CompilationUnit &unit) {
    // Locate all methods
    for (auto method: unit.getAllMethods()) {
        // perform any necessary mangling
        mangleVarNames(*method);
        // flatten it
        auto flat = method->flatten();
        // remove all declarations and push them into a list
        std::vector< InitializeVar * > allInitializations;
        for (unsigned int i = 0; i < flat.size(); i++) {
            auto item = flat[i];
            if (instanceof<InitializeVar>(item)) {
                auto v = (InitializeVar *) item;
                auto copy = new InitializeVar(*v);
                allInitializations.push_back(copy);
                v->active = false;
            }
        }
        // insert all declarations into the top
        for (auto init: allInitializations) {
            pushf(*method, (Statement *) init);
        }
    }
}