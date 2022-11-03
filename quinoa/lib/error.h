#pragma once
#include<string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/prctl.h>

void print_trace();
void error(std::string reason, bool trace=false);

enum ErrorType{
    E_ERR,
    E_INTERNAL,
    E_BAD_ARGS,
    E_UNDECLARED_VAR,
    E_UNRESOLVED_CALL,
    E_BAD_OPERAND,
    E_BAD_ASSIGNMENT,
    E_UNININTIALIZED_CONST,
    E_UNRELATED_TYPE,
    E_UNRESOLVED_TYPE,
    E_NO_ARRAY_LEN,
    E_BAD_ARRAY_LEN,
    E_NO_ENTRYPOINT,

    // Parser-Related Errors
    E_UNESCAPABLE,
    E_UNREADABLE_CHAR,
    E_BAD_TYPE_SYNTAX,
    E_BAD_EXPRESSION,
    E_BAD_CONDITIONAL,
    E_BAD_PARAMETER,
    E_BAD_IMPORT_ALIAS,
    E_UNEXPECTED_TOKEN,

    E_PRIVATE_CALL,
    E_UNREGONIZED_SEED,
    E_BAD_IMPORT,
    E_BAD_MEMBER_ACCESS

};


void except(ErrorType err, std::string message);