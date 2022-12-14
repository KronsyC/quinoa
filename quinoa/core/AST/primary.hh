#pragma once

#include "./ast.hh"
#include "./include.hh"
#include "llvm/IR/Value.h"
#include <map>
#include "../llvm_utils.h"
#include "../../lib/logger.h"

#include "llvm/IR/Type.h"
#include "../../lib/list.h"

class Type;

/**
 * A Convenient wrapper around llvm types
 * Also contains the quinoa type from which it derives from
 */
class LLVMType {
public:
    _Type qn_type;

    llvm::Type *operator->();

    LLVMType(_Type qn_type);
    LLVMType(){
    this->is_explicitly_constructed = false;
  }
    LLVMType(llvm::Type *ll, _Type qn) {
        if(!qn)except(E_INTERNAL, "(bug) cannot construct an LLVMType without the corresponding quinoa type");
        this->cached_type = ll;
        this->qn_type = qn;
    }


    bool operator==(LLVMType& other) const;

    bool is_signed();

    operator bool() const{
      return is_explicitly_constructed;
    }

    operator llvm::Type*() const;
    llvm::Type* get_type() const;
private:
    bool is_explicitly_constructed = true;
    llvm::Type* cached_type = nullptr;
};

class LLVMValue{
public:
    LLVMType type;
    llvm::Value * val;

    LLVMValue(llvm::Value * val, LLVMType type){
        this->val = val;
        this->type = type;
    }

    llvm::Value* operator-> () const{
        return val;
    }
    operator llvm::Value*(){
        return val;
    }
    LLVMValue load();

    void print();
};

class Scope;

enum ReturnChance {
    NEVER,
    MAYBE,
    DEFINITE,
};

class Method;

class Statement : public ANode {
public:
    virtual void generate(Method *qn_fn, llvm::Function *func, VariableTable &vars, ControlFlowInfo CFI) = 0;

    virtual std::string str() = 0;

    virtual std::vector<Statement *> flatten() = 0;

    virtual ReturnChance returns() = 0;
    virtual std::vector<Type*> flatten_types() = 0;
    Scope *scope = nullptr;

};

/**
 * An Expression is similar to a statement, except:
 * 1. It returns a value
 * 2. It is inherently Typed
*/
class Expr : public Statement {
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


    _Type type() {
        // if (recalculate_type || !cached_type) {
            // auto typ = get_type();
            // recalculate_type = false;
            // cached_type = typ;
        // }
        return get_type();
    }

    void generate(Method *qn_fn, llvm::Function *func, VariableTable &vars, ControlFlowInfo CFI) {
        // Generate the expression as a statement
        // This is common for use-cases such as calls (where the function has side effects)
        llvm_value(vars);
    }


    ReturnChance returns() {
        return ReturnChance::NEVER;
    }

    virtual LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) = 0;

    virtual LLVMValue assign_ptr(VariableTable &vars) = 0;


protected:
    virtual _Type get_type() = 0;

    Expr *parent_expr = nullptr;
private:
};


enum JumpType {
    BREAK,
    CONTINUE,
    FALLTHROUGH,
};

class ControlFlowJump : public Statement {
public:
    JumpType type;

    ControlFlowJump(JumpType type) {
        this->type = type;
    }
    std::vector<Type*> flatten_types(){return {};}
    void generate(Method *qn_fn, llvm::Function *func, VariableTable &vars, ControlFlowInfo CFI) {
        switch (type) {
            case JumpType::BREAK: {
                if (!CFI.breakTo)except(E_BAD_CONTROL_FLOW, "Cannot break from a non-breakable scope");
                builder()->CreateBr(CFI.breakTo);
                break;
            }
            case JumpType::CONTINUE: {
                if (!CFI.continueTo)except(E_BAD_CONTROL_FLOW, "Cannot continue from a non-continuable scope");
                builder()->CreateBr(CFI.continueTo);
                break;
            }
            case JumpType::FALLTHROUGH: {
                if (!CFI.fallthroughTo)
                    except(E_BAD_CONTROL_FLOW, "Cannot fallthrough from a non-fallthroughable scope");
                builder()->CreateBr(CFI.fallthroughTo);
                break;
            }
            default: except(E_INTERNAL, "Bad control flow jump type");
        }
    }

    std::string str() {
        switch (type) {
            case BREAK: return "break";
            case CONTINUE: return "continue";
            case FALLTHROUGH: return "fallthrough";
            default: return "unknown_cfj_op";
        }
    }

    std::vector<Statement *> flatten() {
        return {this};
    }

    ReturnChance returns() {
        return ReturnChance::NEVER;
    }
};


/**
 * A 'scope' represents the contents of a braced block
 */
#include<regex>

class Scope : public Statement {
public:
    Vec<Statement> content;
    
    std::vector<Type*> flatten_types();

    std::string str() {
        std::string output = "{\n";
        for (auto item: content) {
            auto str = "   " + item->str();
            str = std::regex_replace(str, std::regex("\n"), "\n   ");
            output += str + ";";
            output += "\n";
        }
        return output + "}";
    }

    void generate(Method *qn_fn, llvm::Function *func, VariableTable &vars, ControlFlowInfo CFI);
    std::vector<Statement *> flatten() {
        std::vector< Statement * > ret = {this};
        for (auto c: content)for (auto m: c->flatten())ret.push_back(m);
        return ret;
    }

    void set_type(std::string var_name, _Type typ) {
        type_table[var_name] = typ;
    }

    _Type get_type(std::string var_name) {
        auto lookup = type_table[var_name];
        if (lookup)return lookup;
        else if (scope)return scope->get_type(var_name);
        else {
            except(E_UNRESOLVED_TYPE, "Failed to get type of '" + var_name + "'", false);
            return _Type(nullptr);
        }
    }

    ReturnChance returns() {

        bool maybe_return = false;
        for (auto item: content) {

            if (dynamic_cast<ControlFlowJump *>(item.ptr))return ReturnChance::NEVER;

            if (item->returns() == ReturnChance::MAYBE)maybe_return = true;
            if (item->returns() == ReturnChance::DEFINITE)return ReturnChance::DEFINITE;
        }

        return maybe_return ? ReturnChance::MAYBE : ReturnChance::NEVER;
    }

    Statement* parent_of(Statement* node){
        auto content = this->flatten();
        if(!includes(content, node))return nullptr;

        size_t check_idx = indexof(content, node) - 1;

        while(check_idx >= 0){
            auto current_node = content[check_idx];
            auto current_children = current_node->flatten();
            if(includes(current_children, node))return current_node;
            check_idx--;
        }
        return nullptr;
    }

    void decl_new_variable(std::string name, _Type type, bool is_constant = false);
private:
    std::map <std::string, std::unique_ptr<Variable>> vars;
    std::map <std::string, _Type> type_table;
};
