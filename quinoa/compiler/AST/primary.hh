#pragma once
#include "./ast.hh"
#include "./include.hh"
#include "llvm/IR/Value.h"
#include "./type.hh"
#include <map>

#define ContextType std::shared_ptr<Context>



//
// Contexts are strongly tied to scopes
//
class Context{
public:
    //
    // Keep track of the types accessible by members of the context
    //
    std::map<std::string, std::unique_ptr<Type>> type_table;
    ContextType parent;


    Type& get_type(std::string var_name){
        auto my_type = type_table[var_name].get();
        if(my_type)return *my_type;
        else if(parent)return parent->get_type(var_name);
        else except(E_UNRESOLVED_TYPE, "Failed to read the type of '" + var_name +"'");
    }
};


class Statement : public ANode{
public:
    virtual void generate() = 0;
    virtual std::string str() = 0;
};

/**
 * An Expression is similar to a statement, except:
 * 1. It returns a value
 * 2. It is inherently Typed
*/
class Expr : public Statement{
public:
    /**
     * The parent of the expression (if applicable)
     * 
     * I.E
     * 
     *    +
     *   / \
     *  1   2
     * 
     * in this case, '+' has no parent (nullptr)
     * '1', and '2' have the common parent of '+'
    */


    Type& type(){
        if(recalculate_type){
            auto typ = get_type();
            recalculate_type = false;
            cached_type = std::move(typ);
        }
        if(!cached_type)except(E_INTERNAL, "Type is already calculated, yet the cache is empty");
        return *cached_type;
    }
    void generate(){
        // Generate the expression as a statement
        // This is common for use-cases such as calls (where the function has side-effects)
        llvm_value();
    }
    Expr& get_parent(){
        return *this->parent_expr;
    }
    virtual llvm::Value* llvm_value() = 0;
    
protected:
    virtual std::unique_ptr<Type> get_type() = 0;

    /**
     * Signify to the expression that a dependency has been changed,
     * and some attributes must be recalculated
    */
    void changed_dep(){
        recalculate_type = true;
        if(parent_expr)parent_expr->changed_dep();
    }
    Expr* parent_expr = nullptr;
private:
    bool recalculate_type = true;
    std::unique_ptr<Type> cached_type;
};


/**
 * A 'scope' represents the contents of a braced block
 */
class Scope: public Statement{
public:
private:
    Vec<Statement> content;  
};