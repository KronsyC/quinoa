#pragma once

#include "./ast.hh"
#include "./include.hh"
#include "llvm/IR/Value.h"
#include <map>
#include "../llvm_utils.h"
#include "../../lib/logger.h"

#include "llvm/IR/Type.h"

class Type;

/**
 * A Convenient wrapper around llvm types
 * Also contains the quinoa type from which it derives from
 */
class LLVMType {
public:
    llvm::Type *ll_type = nullptr;
    Type *qn_type = nullptr;

    llvm::Type *operator->() {
        return this->ll_type;
    }

    LLVMType(llvm::Type *r) {
        this->ll_type = r;
    }

    LLVMType() = default;

    LLVMType(llvm::Type *ll, Type *qn) {
        this->ll_type = ll;
        this->qn_type = qn;
    }

    operator llvm::Type *() const {
        return ll_type;
    }


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


    std::shared_ptr <Type> type() {
        if (recalculate_type || !cached_type) {
            auto typ = get_type();
            recalculate_type = false;
            cached_type = typ;
        }
        return cached_type;
    }

    void generate(Method *qn_fn, llvm::Function *func, VariableTable &vars, ControlFlowInfo CFI) {
        // Generate the expression as a statement
        // This is common for use-cases such as calls (where the function has side effects)
        llvm_value(vars);
    }

    Expr &get_parent() {
        return *this->parent_expr;
    }

    ReturnChance returns() {
        return ReturnChance::NEVER;
    }

    virtual llvm::Value *llvm_value(VariableTable &vars, LLVMType expected_type = {}) = 0;

    virtual llvm::Value *assign_ptr(VariableTable &vars) = 0;

protected:
    virtual std::shared_ptr <Type> get_type() = 0;

    /**
     * Signify to the expression that a dependency has been changed,
     * and some attributes must be recalculated
    */
    void changed_dep() {
        recalculate_type = true;
        if (parent_expr)parent_expr->changed_dep();
    }

    Expr *parent_expr = nullptr;
private:
    bool recalculate_type = true;
    std::shared_ptr <Type> cached_type;
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

    void generate(Method *qn_fn, llvm::Function *func, VariableTable &vars, ControlFlowInfo CFI) {
        for (auto &child: content) {
            Logger::debug("Generating Child:\n" + child->str());
            child->generate(qn_fn, func, vars, CFI);
        }
    }

    std::vector<Statement *> flatten() {
        std::vector < Statement * > ret = {this};
        for (auto c: content)for (auto m: c->flatten())ret.push_back(m);
        return ret;
    }

    void set_type(std::string var_name, std::shared_ptr <Type> typ) {
        type_table[var_name] = typ;
    }

    std::shared_ptr <Type> get_type(std::string var_name) {
        auto lookup = type_table[var_name];
        if (lookup)return lookup;
        else if (scope)return scope->get_type(var_name);
        else {
            except(E_UNRESOLVED_TYPE, "Failed to get type of '" + var_name + "'", false);
            return std::shared_ptr<Type>(nullptr);
        }
    }

    ReturnChance returns() {


        for (auto item: content) {


            if (dynamic_cast<ControlFlowJump *>(item.ptr))return ReturnChance::NEVER;

            if (item->returns() == ReturnChance::MAYBE)return ReturnChance::MAYBE;
            if (item->returns() == ReturnChance::DEFINITE)return ReturnChance::DEFINITE;
        }

        return ReturnChance::NEVER;
    }

private:
    std::map <std::string, std::shared_ptr<Type>> type_table;
};