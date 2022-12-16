/**
 * Ensure that variables are only used after they are declared
*/

#pragma once

#include "../include.h"


void validate_usage(Block &block, std::vector<std::string> declared_vars) {
    for (auto member: block.flatten()) {
        if (member->scope == &block) {
            if (auto nest = dynamic_cast<Block *>(member)) {
                validate_usage(*nest, declared_vars);
                continue;
            }
            if (auto init = dynamic_cast<InitializeVar *>(member)) {
                declared_vars.push_back(init->var_name.str());
            }
            // TODO: Implement checks for usage before declaration
            //       the old implementation was removed as it does not
            //       act nicely with structs (flattening `x.y` results
            //       in `y` being interpreted as a standalone variable)


        }
    }
}

void validate_variable_usage(CompilationUnit &unit) {
    for (auto method: unit.get_methods()) {
        if (!method->content)continue;
        std::vector<std::string> declarations;
        for (auto &p: method->parameters) {
            declarations.push_back(p->name.str());
        }
        for (auto prop: unit.get_properties()) {
            declarations.push_back(prop->name->str());

            if (prop->name->container == method->name->container) {
                declarations.push_back(prop->name->member->str());
            }
        }

        validate_usage(*method->content, declarations);
    }
}
