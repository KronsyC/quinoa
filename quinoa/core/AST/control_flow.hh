#pragma once

#include "./include.hh"
#include "./primary.hh"
#include "type.hh"

class Conditional : public Statement {
  public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Block> if_true;
    std::unique_ptr<Block> if_false;

    std::vector<Type*> flatten_types() {
        auto ret = condition->flatten_types();
        for (auto t : if_true->flatten_types())
            ret.push_back(t);
        if (if_false)
            for (auto t : if_false->flatten_types())
                ret.push_back(t);

        return ret;
    }

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        for (auto m : condition->flatten())
            ret.push_back(m);
        for (auto m : if_true->flatten()) {
            ret.push_back(m);
        }
        if (if_false)
            for (auto m : if_false->flatten())
                ret.push_back(m);
        return ret;
    }

    void generate(Method* qn_fn, llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI) {

        auto eval_if = condition->llvm_value(vars, Primitive::get(PR_boolean)->llvm_type());
        auto true_block = llvm::BasicBlock::Create(*llctx(), "if_true", func);
        auto false_block = if_false ? llvm::BasicBlock::Create(*llctx(), "if_false", func)
                                    : llvm::BasicBlock::Create(*llctx(), "if_cont", func);
        auto cont_block = if_false ? llvm::BasicBlock::Create(*llctx(), "if_cont", func) : false_block;

        builder()->CreateCondBr(eval_if, true_block, false_block);

        builder()->SetInsertPoint(true_block);

        if_true->generate(qn_fn, func, vars, CFI);

        if (if_true->returns() != ReturnChance::DEFINITE)
            builder()->CreateBr(cont_block);

        builder()->SetInsertPoint(false_block);

        if (if_false)
            if_false->generate(qn_fn, func, vars, CFI);
        if (if_false && if_false->returns() != ReturnChance::DEFINITE)
            builder()->CreateBr(cont_block);

        builder()->SetInsertPoint(cont_block);

        // dangling continue block, can safely remove
        if (returns() == ReturnChance::DEFINITE) {
            cont_block->removeFromParent();
            delete cont_block;
        }
    }

    std::string str() {

        std::string out = "if( " + condition->str() + ") ";
        out += if_true->str();
        if (if_false) {

            out += "\nelse" + if_false->str();
        }
        return out;
    }

    bool is_container() { return true; }

    ReturnChance returns() {
        if (!if_false)
            return ReturnChance::MAYBE;
        else {
            auto r1 = if_true->returns();
            auto r2 = if_false->returns();

            if (r1 == ReturnChance::MAYBE || r2 == ReturnChance::MAYBE)
                return ReturnChance::MAYBE;
            if (r1 == ReturnChance::DEFINITE && r2 == ReturnChance::DEFINITE)
                return ReturnChance::DEFINITE;
            if (r1 == ReturnChance::DEFINITE || r2 == ReturnChance::DEFINITE)
                return ReturnChance::MAYBE;
            return ReturnChance::NEVER;
        }
    }
};

class While : public Statement {
  public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Block> execute;

    std::vector<Type*> flatten_types() {
        auto ret = condition->flatten_types();
        for (auto t : execute->flatten_types())
            ret.push_back(t);

        return ret;
    }

    void generate(Method* qn_fn, llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI) {

        auto eval_block = llvm::BasicBlock::Create(*llctx(), "while_eval", func);
        auto exec_block = llvm::BasicBlock::Create(*llctx(), "while_exec", func);
        auto cont_block = llvm::BasicBlock::Create(*llctx(), "while_cont", func);

        CFI.breakTo = cont_block;
        CFI.continueTo = eval_block;
        CFI.exitBlock = cont_block;

        builder()->CreateBr(eval_block);

        builder()->SetInsertPoint(eval_block);
        auto br_if = condition->llvm_value(vars, Primitive::get(PR_boolean)->llvm_type());
        builder()->CreateCondBr(br_if, exec_block, cont_block);

        builder()->SetInsertPoint(exec_block);
        execute->generate(qn_fn, func, vars, CFI);
        builder()->CreateBr(eval_block);

        builder()->SetInsertPoint(cont_block);
    }

    std::vector<Statement*> flatten() {
        std::vector<Statement*> ret = {this};
        for (auto m : condition->flatten())
            ret.push_back(m);
        for (auto m : execute->flatten())
            ret.push_back(m);
        return ret;
    }

    std::string str() { return "while( " + condition->str() + ")" + execute->str(); }

    bool is_container() { return true; }

    ReturnChance returns() { return execute->returns(); }
};
