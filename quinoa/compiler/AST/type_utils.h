#pragma once
#include "./type.hh"

namespace TypeUtils{
    std::unique_ptr<Type> get_common_type(Type& type_1, Type& type_2);
};