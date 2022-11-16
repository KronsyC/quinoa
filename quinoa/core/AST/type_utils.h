#pragma once
#include <memory>
#include <vector>
class Type;

namespace TypeUtils{
    std::shared_ptr<Type> get_common_type(std::shared_ptr<Type> type_1, std::shared_ptr<Type> type_2, bool second_pass = false);
    std::shared_ptr<Type> get_common_type(std::vector< std::shared_ptr<Type> > types);
};