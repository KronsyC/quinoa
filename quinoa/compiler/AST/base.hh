#pragma once
#include <vector>
#include <string>
#include "../../lib/error.h"
class AstNode
{
public:
    virtual ~AstNode() = default;
};

struct Statement : public AstNode
{
    virtual std::vector<Statement *> flatten()
    {
        error("Cannot Flatten a raw Statement");
        return {};
    }
};
struct Type : public AstNode
{
public:
    virtual std::string str(){
        error("Cannot stringify base type");
        return "";
    }

    virtual bool equals(Type* type){
        error("Cannot Check Type Equality from base class");
        return false;
    }
};
struct Expression : public Statement
{
public:
    virtual Type *getType()
    {
        error("GetType Fn is not implemented for Expression");
        return nullptr;
    };
};
template <typename T>
class Block : public AstNode
{
public:
    std::vector<T *> items;

    size_t push(T *item)
    {
        items.push_back(item);
        return items.size();
    }

    std::vector<T *> take()
    {
        destroy = false;
        return this->items;
    }

private:
    bool destroy = true;
};

struct TopLevelExpression : public AstNode
{
};
struct ModuleMember : public AstNode
{
};
#include "./identifier.hh"
class CompilationUnit : public Block<TopLevelExpression>
{
};
