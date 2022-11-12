#include "./clarg_parser.hh"
#include "logger.h"

template<>
void ClargParser::add_clarg(std::string name, std::string desc, int default_value){
    ExpectedClarg arg;
    arg.description = desc;
    arg.required = false;
    arg.type = ClargParser::ArgType::INT;
    arg.value = new int(default_value);
    arg.initialized = true;
    kw_args[name] = arg;
}

template<>
void ClargParser::add_clarg(std::string name, std::string desc, std::string default_value){
    ExpectedClarg arg;
    arg.description = desc;
    arg.required = false;
    arg.type = ClargParser::ArgType::STR;
    arg.value = new std::string(default_value);
    arg.initialized = true;
    kw_args[name] = arg;
}


template<>
std::string ClargParser::get_clarg(std::string name){
    auto entry = kw_args[name];
    if(!entry.initialized)except(E_INTERNAL, "Attempted to get arg " + name + ", but it does not exist");
    if(entry.type != ClargParser::ArgType::STR)except(E_INTERNAL, "Attempted to get arg " + name + " as a string, which is the wrong type");
    return *(std::string*)entry.value;
}