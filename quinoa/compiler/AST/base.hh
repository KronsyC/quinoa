#pragma once
#include <vector>
#include <string>
#include "../../lib/error.h"
#include "llvm/IR/Type.h"
class AstNode
{
public:
    virtual ~AstNode() = default;
};

class SourceBlock;

template<class Class, typename KeyType>
class Cached{
protected:
    inline static std::map<KeyType, Class*> cache;
};

struct Statement : public AstNode
{
public:
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
    virtual bool returns(){
        return false;
    };
};
struct Type : public AstNode
{
public:
    virtual std::string str(){
        error("Cannot stringify base type");
        return "";
    }
    virtual llvm::Type* getLLType(){
        error("Cannot get LL Type for base Type Class");
        return nullptr;
    }
};

struct Expression : public Statement
{
public:
    virtual Type *getType()
    {
        error("GetType Fn is not implemented for Expression", true);
        return nullptr;
    };

    virtual llvm::Value* getLLValue(TVars vars, llvm::Type* expected=nullptr){
        error("Cannot get llvm value for base expression type", true);
        return nullptr;
    }
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
    ~Block(){
        if(destroy){
        for(auto i:items){
            delete i;
        }
        }

    }

private:
    bool destroy = true;
};

struct TopLevelExpression : public AstNode
{
public:
    virtual bool isModule(){
        return false;
    }
    virtual bool isImport(){
        return false;
    }
    bool isImported = false;

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
    SourceBlock(std::vector<Statement*> items){
        this->items = items;
        this->local_types = new LocalTypeTable;
    }
    SourceBlock(Statement* item){
        this->items.push_back(item);
        this->local_types = new LocalTypeTable;
    }
    ~SourceBlock(){
        delete local_types;
    }
    bool returns(){
        for(auto item:items){
            if(item->returns())return true;
        }
        return false;
    }
    SourceBlock() = default;
    LocalTypeTable* local_types = nullptr;
    std::vector<std::string> declarations;

    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret;
        for(auto i:items){
            for(auto m:i->flatten()){
                ret.push_back(m);

            }
        }
        return ret;
    }


    void insert(SourceBlock* donor){
        if(donor==nullptr)error("Cannot merge with a null donor");
        auto old = donor;
        for(auto item:donor->take()){
            // update the ctx
            Statement* prev;
            for(auto i:item->flatten()){
                if(i->ctx == old){
                    i->ctx = this;
                }
                prev = i;
            }
            item->ctx = this;
            items.push_back(item);
        }
        for(auto pair:*donor->local_types){
            if(local_types==nullptr)error("My locals are null?");
            auto my = *local_types;
            if(my[pair.first] == nullptr){
                (*local_types)[pair.first] = pair.second;
            }

        }
        delete donor;
    }
};
