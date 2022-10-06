#pragma once
#include <map>
#include <string>
#define LocalTypeTable std::map<std::string, Type*>


template <typename Base, typename T>
inline bool instanceof (T * item)
{
    return dynamic_cast<Base *>(item) != nullptr;
}



#include "../../lib/logger.h"

#include "./base.hh"
#include "./identifier.hh"
#include "./type.hh"
#include "./module.hh"
#include "./constant.hh"
#include "./operations.hh"
#include "./control_flow.hh"

