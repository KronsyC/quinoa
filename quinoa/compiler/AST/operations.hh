#pragma once
#include "./ast.hh"
#include<vector>
#include<map>
#include "../token/TokenDef.h"


class MethodCall:public Expression{
public:
    MethodSignature* target;
    CompoundIdentifier* name;
    std::vector<Expression*> params;

    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret{this, name};
        for(auto p:params)ret.push_back(p);
        return ret;
    }
};

class Return: public Statement{
public:
    Expression* retValue;
    Return(Expression* value){
        this->retValue = value;
    }
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret{this};
        for(auto i:retValue->flatten())ret.push_back(i);
        return ret;
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

    std::vector<Statement*> flatten(){
        std::vector<Statement*>flat;
        auto l = left->flatten();
        for(auto i:l)flat.push_back(i);
        auto r = right->flatten();
        for(auto i:r)flat.push_back(i);
        return flat;
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
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret{this};
        for(auto i:varname->flatten())ret.push_back(i);
        return ret;
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
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret{tgt};
        for(auto p:item->flatten())ret.push_back(p);
        return ret;
    }
};