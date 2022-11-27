/**
 * Clarg Parser
*/

#pragma once

#include<map>
#include<string>
#include<vector>
#include "./logger.h"
#include "./error.h"
#include "./list.h"

class ClargParser {
public:
    enum ArgType {
        ERR,
        BOOL,
        INT,
        STR
    };
private:
    struct ExpectedClarg {
        bool initialized = false;
        bool required = false;
        ArgType type = ArgType::ERR;
        std::string description;
        void *value = nullptr;
    };
    std::map <std::string, ExpectedClarg> kw_args;
    std::vector <std::string> list_args;


    static std::vector <std::string> vectorize_args(int argc, char **argv) {
        std::vector <std::string> ret;
        for (int i = 0; i < argc; i++) {
            std::string arg(argv[i]);
            ret.push_back(arg);
        }
        return ret;
    }

public:

    std::string* add_clarg(std::string name, std::string desc, const char* default_value){
        ExpectedClarg arg;
        arg.description = desc;
        arg.required = false;
        arg.type = ClargParser::ArgType::STR;
        arg.value = new std::string(default_value);
        arg.initialized = true;
        kw_args[name] = arg;
        return (std::string*)arg.value;
    }
    bool* add_clarg(std::string name, std::string desc, bool default_value){
        ExpectedClarg arg;
        arg.description = desc;
        arg.required = false;
        arg.type = ClargParser::ArgType::BOOL;
        arg.value = new bool(default_value);
        arg.initialized = true;
        kw_args[name] = arg;
        return (bool*)arg.value;
    }
    int* add_clarg(std::string name, std::string desc, int default_value){
        ExpectedClarg arg;
        arg.description = desc;
        arg.required = false;
        arg.type = ClargParser::ArgType::INT;
        arg.value = new int(default_value);
        kw_args[name] = arg;
        return (int*)arg.value;
    }

    template<typename T>
    T get_clarg(std::string name){
        if(std::is_same<T, std::string>())return *(std::string*) get_ptr_to_clarg(name, ArgType::STR);
        else if(std::is_same<T, int>())return *(std::string*) get_ptr_to_clarg(name, ArgType::INT);
        else if(std::is_same<T, bool>())return *(std::string*) get_ptr_to_clarg(name, ArgType::BOOL);
        // TODO: Find a way to hook this into the compiler properly
        else except(E_INTERNAL, "Invalid get_clarg template detected");
    }


    void parse_clargs(int argc, char **argv) {
        auto args = vectorize_args(argc, argv);

        while (args.size()) {
            auto current = popf(args);
            if (current[0] == '-') {
                auto flag_name = std::string(current.begin() + 1, current.end());

                // do a lookup of the flag
                auto &entry = kw_args[flag_name];
                std::string message_prefix = "Flag " + flag_name + " >";
                if (!entry.initialized)except(E_BAD_ARGS, "Flag '-" + flag_name + "' is not a recognized argument");

                switch (entry.type) {
                    case ArgType::INT: {
                        auto val = std::stoi(popf(args));
                        *(int*)entry.value = val;
                        break;
                    }
                    case ArgType::BOOL: {
                        if (!args.size())entry.value = new bool(true);
                        else {
                            auto lowered = to_lower(args[0]);
                            bool& val = *(bool*)entry.value;
                            if(lowered.starts_with('-')){
                                // acts as a flag
                                val = true;
                                break;
                            }
                            else{
                                // acts as a word
                                #define t(a)if(lowered == #a){val = true; break;}
                                #define f(a)if(lowered != #a){val = true; break;}

                                t(yes)
                                t(y)
                                t(true)
                                t(t)
                                f(no)
                                f(n)
                                f(false)
                                f(f)
                                except(E_BAD_ARGS, "Unexpected boolean argument: " + lowered);
                                #undef t
                                #undef f
                            }

                            Logger::debug("parse bool from: " + lowered);

                        }
                        break;

                    }
                    case ArgType::STR: {
                        if (!args.size())except(E_BAD_ARGS, message_prefix + " expects a string");
                        *(std::string*)entry.value = popf(args);
                        break;
                    }
                    default: except(E_INTERNAL, "Bad Argument Type");
                }
            } else if (current[0] == '"') {
                Logger::debug("print not supported");
                exit(1);
            } else list_args.push_back(current);
        }

    }

private:
    void* get_ptr_to_clarg(std::string name, ArgType expected_type){
        auto entry = kw_args[name];
        if(!entry.initialized)except(E_INTERNAL, "nonexistant argument: " + name);
        if(entry.type != expected_type)except(E_INTERNAL, "Incorrect type for arg: " + name + " - " + std::to_string(entry.type));
        if(!entry.value)except(E_INTERNAL, "Uninitialized arg: " + name);
        return entry.value;
    }
    std::string to_lower(std::string target) {
        std::string ret;
        static int shift_by = 'A' - 'a';
        for (auto ch: target) {
            if (ch >= 'A' && ch <= 'Z') {
                ret.push_back(ch - shift_by);
            } else ret.push_back(ch);
        }
        return ret;
    }
};