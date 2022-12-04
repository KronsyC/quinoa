#pragma once
#include "../../../lib/logger.h"
#include "../../../lib/list.h"

#include "../../AST/ast.hh"
#include "../../AST/reference.hh"
#include "../../AST/advanced_operators.hh"
#include "../../AST/container.hh"
#include "../../AST/container_member.hh"
#include "../../AST/compilation_unit.hh"
#include "../../AST/type.hh"

#include<optional>
#include<vector>
#include<string>

#define MaybeError std::optional<std::vector<std::string>>

#define proc_result std::pair<unsigned int, MaybeError>