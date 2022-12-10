#pragma once
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
void print_trace();
void error(std::string reason, bool trace = false);

#define ERR_TYPES          \
    X(ERR)                 \
    X(NOPREFIX)             \
    X(INTERNAL)            \
    X(BAD_ARGS)            \
    X(BAD_METHOD_DEFINITION) \
    X(BAD_CONTROL_FLOW)    \
    X(BAD_SUBSTITUTION)                       \
    X(BAD_COMPOSITOR)      \
    X(BAD_INDEX)           \
    X(BAD_INTRINSIC_CALL)  \
    X(BAD_METADATA)        \
    X(BAD_VAR)             \
    X(BAD_RETURN)          \
    X(UNDECLARED_VAR)      \
    X(UNRESOLVED_CALL)     \
    X(BAD_OPERAND)         \
    X(BAD_CALL)            \
    X(BAD_ASSIGNMENT)      \
    X(CONST_ASSIGNMENT)    \
    X(UNINITIALIZED_CONST) \
    X(UNDEFINED_PROPERTY)  \
    X(BAD_CAST)            \
    X(NONEQUIVALENT_TYPES) \
    X(UNRESOLVED_TYPE)     \
    X(NO_ARRAY_LEN)        \
    X(BAD_ARRAY_LEN)       \
    X(NO_ENTRYPOINT)       \
    X(MODULE_INHERITANCE)  \
    X(UNESCAPABLE)         \
    X(UNEXPECTED_DECIMAL)  \
    X(BAD_ESCAPE)          \
    X(UNREADABLE_CHAR)     \
    X(BAD_TYPE)            \
    X(BAD_EXPRESSION)      \
    X(BAD_CONDITIONAL)     \
    X(BAD_PARAMETER)       \
    X(BAD_IMPORT_ALIAS)    \
    X(UNEXPECTED_TOKEN)    \
    X(PRIVATE_CALL)        \
    X(UNRECOGNIZED_SEED)   \
    X(BAD_IMPORT)          \
    X(BAD_MEMBER_ACCESS)   \
    X(MISSING_FUNCTION)

#define X(ename) E_##ename,

enum ErrorType { ERR_TYPES };

#undef X

[[noreturn]]void except(ErrorType err, std::string message);
void except(ErrorType err, std::string message, bool exits);
