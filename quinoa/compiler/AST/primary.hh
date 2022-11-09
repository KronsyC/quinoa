#pragma once
#include "./ast.hh"
#include "./include.hh"
#include "llvm/IR/Value.h"
#include "./type.hh"
#include <map>
#include "../llvm_globals.h"

class Scope;

class Statement : public ANode{
public:
    virtual void generate(llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI) = 0;
    virtual std::string str() = 0;
    virtual std::vector<Statement*> flatten() = 0;
    Scope* scope = nullptr;
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
    void generate(llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI){
        // Generate the expression as a statement
        // This is common for use-cases such as calls (where the function has side-effects)
        llvm_value(vars);
    }
    Expr& get_parent(){
        return *this->parent_expr;
    }
    virtual llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected_type = nullptr) = 0;
    
protected:
    virtual std::shared_ptr<Type> get_type() = 0;

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
    std::shared_ptr<Type> cached_type;
};


/**
 * A 'scope' represents the contents of a braced block
 */
#include<regex>
class Scope: public Statement{
public:
    Vec<Statement> content;  

    std::string str(){
        std::string output = "{\n";
        for(auto item : content){
            auto str = item->str();
            str = std::regex_replace(str, std::regex("\n"), "\n\t");
            output += str;
            output+="\n";
        }
        return output + "\n}";
    }

    void generate(llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI){
        for(auto& child : content){
            child->generate(func, vars, CFI);
        }
    }
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto c: content)for(auto m : c->flatten())ret.push_back(m);
        return ret;
    }
    
};