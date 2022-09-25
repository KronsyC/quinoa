#pragma once
#include "./ast.hh"
#include<vector>
#include<map>
#include "../token/TokenDef.h"


class MethodCall:public Expression{
public:
    Identifier* target;
    std::vector<Expression*> params;
};

class Return: public Statement{
public:
    Expression* retValue;
    Return(Expression* value){
        this->retValue = value;
    }
};

enum BinaryOp{
    INFIX_ENUM_MEMBERS
};

static std::map<TokenType, BinaryOp> binary_op_mappings{
    INFIX_ENUM_MAPPINGS
};
class BinaryOperation:public Expression{
public:
    Expression* left;
    Expression* right;
    BinaryOp op;

    BinaryOperation(Expression* left, Expression* right, BinaryOp op){
        this->left = left;
        this->right = right;
        this->op = op;
    }
};

class InitializeVar:public Statement{
public:
    Type* type;
    Identifier* varname;
    InitializeVar(Type* t, Identifier* name){
        type = t;
        varname = name;
    }
};

class Subscript:public Expression{
public:
    Identifier* tgt;
    Expression* item;
    Subscript(Identifier* tgt, Expression* item){
        this->tgt = tgt;
        this->item = item;
    }
};