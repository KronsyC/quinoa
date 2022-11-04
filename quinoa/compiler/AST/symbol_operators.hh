/**
 * Symbol (Simple) operators, i.e builtin Binary and Unary operations
*/

#pragma once
#include "./primary.hh"
#include "./type.hh"
#include "../../GenMacro.h"


enum UnaryOpType{
    UNARY_ENUM_MEMBERS
};

enum BinaryOpType{
    INFIX_ENUM_MEMBERS
};



class UnaryOperation: public Expr{
public:
    std::unique_ptr<Expr> operand;
    UnaryOpType op_type;

private:
    std::unique_ptr<Type> get_type(){
        auto bool_t = Primitive::get(PR_boolean);
        auto same_t = std::make_unique<Type>(operand->type());
        auto same_ptr = Ptr::get(same_t);
    }
};

class BinaryOperation: public Expr{
public:
    std::unique_ptr<Expr> left_operand;
    std::unique_ptr<Expr> right_operand;
    BinaryOpType op_type;
};


