#pragma once
#include <vector>
#include <string>
#include "../../lib/error.h"
class AstNode
{
public:
    virtual ~AstNode() = default;
};

class SourceBlock;

struct Statement : public AstNode
{
    // Statements can be deactivated if needed
    // This is useful when working with flattened asts, where it is impossible to remove nodes
    // The next best thing is deactivation
    bool active = true;
    SourceBlock* ctx = nullptr;
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
    virtual Type *getType(LocalTypeTable type_table)
    {
        error("GetType Fn is not implemented for Expression", true);
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

// A Souceblock is a wrapper around a statement block
// but contains important information such as guarantees about execution
// and a local scope type table
// variables cannot be redefined within the same block and are hence guaranteed to keep the same type
class SourceBlock:public Block<Statement>{
public:
    LocalTypeTable* local_types;
    std::vector<std::string> declarations;
    // This is messy, im sorry
    SourceBlock* self = this;
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret;
        for(auto i:items){
            auto flatChild = i->flatten();
            for(auto m:i->flatten()){
                ret.push_back(m);

            }
        }
        return ret;
    }
};
