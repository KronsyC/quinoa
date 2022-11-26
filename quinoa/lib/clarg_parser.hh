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
        std::string description = "";
        void *value = nullptr;
    };
    std::map <std::string, ExpectedClarg> kw_args;
    std::vector <std::string> list_args;


    std::vector <std::string> vectorize_args(int argc, char **argv) {
        std::vector <std::string> ret;
        for (int i = 0; i < argc; i++) {
            std::string arg(argv[i]);
            ret.push_back(arg);
        }
        return ret;
    }

public:


    template<ArgType typ>
    void add_clarg(std::string name, std::string description) {
        /* Impl goes here */
    }

    template<typename T>
    void add_clarg(std::string name, std::string description, T default_value);

    template<typename T>
    T get_clarg(std::string name);

    void parse_clargs(int argc, char **argv) {
        auto args = vectorize_args(argc, argv);

        while (args.size()) {
            auto current = popf(args);
            if (current[0] == '\'') {
                auto flag_name = std::string(current.begin() + 1, current.end());

                // do a lookup of the flag
                auto &entry = kw_args[flag_name];
                if (!entry.initialized)except(E_BAD_ARGS, "Flag '-" + flag_name + "' is not a recognized argument");
                auto message_prefix = "Flag: '" + flag_name + "'";
                Logger::debug(message_prefix);

                switch (entry.type) {
                    case ArgType::INT: {
                        auto val = std::stoi(popf(args));
                        entry.value = new int(val);
                        break;
                    }
                    case ArgType::BOOL: {
                        if (!args.size())entry.value = new bool(true);
                        else {
                            auto lowered = to_lower(args[0]);
                            Logger::debug("parse bool from: " + lowered);

                        }
                        break;

                    }
                    case ArgType::STR: {
                        if (!args.size())except(E_BAD_ARGS, message_prefix + " expects a string");
                        if (args[0][0] == '"')except(E_INTERNAL, "complex string arg parsing is not implemented");
                        else {
                            auto val = new std::string(popf(args));
                            entry.value = val;
                        }
                        break;
                    }
                    default: except(E_INTERNAL, "Bad Argument Type");
                }
            } else if (current[0] == '"') {
                Logger::debug("str not supported");
                exit(1);
            } else list_args.push_back(current);
        }

    }

private:
    std::string to_lower(std::string target) {
        std::string ret;
        int shift_by = 'A' - 'a';
        for (auto ch: target) {
            if (ch >= 'A' && ch <= 'Z') {
                ret.push_back(ch - shift_by);
            } else ret.push_back(ch);
        }
        return ret;
    }
};