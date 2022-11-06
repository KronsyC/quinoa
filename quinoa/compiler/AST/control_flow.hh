#pragma once
#include "./include.hh"
#include "./primary.hh"



class Conditional : public Statement{
public:
    std::unique_ptr<Expr>  condition;
    std::unique_ptr<Scope> if_true;
    std::unique_ptr<Scope> if_false;

    void generate(llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI){

        auto eval_if = condition->llvm_value(vars, builder()->getInt1Ty());

        auto true_block  = llvm::BasicBlock::Create(*llctx(), "if_true", func);
        auto false_block = llvm::BasicBlock::Create(*llctx(), "if_false", func);
        auto cont_block  = llvm::BasicBlock::Create(*llctx(), "if_cont", func);

        builder()->CreateCondBr(eval_if, true_block, false_block);


        builder()->SetInsertPoint(true_block);

        if_true->generate(func, vars, CFI);
        builder()->CreateBr(cont_block);


        builder()->SetInsertPoint(false_block);

        if(if_false)if_false->generate(func, vars, CFI);
        builder()->CreateBr(cont_block);


        builder()->SetInsertPoint(cont_block);

    }

    std::string str(){

        std::string out = "if( " + condition->str() + ")";
        out += if_true->str();
        if(if_false){

            out += "else " + if_false->str(); 
        }
        return out;


    }
};

class While : public Statement{
public:
    std::unique_ptr<Expr>  condition;
    std::unique_ptr<Scope> execute;

    void generate(llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI){
        except(E_INTERNAL, "generate() not implemented for 'While' node");
    }

};