/**
 * Symbol (Simple) operators, i.e builtin Binary and Unary operations
 */

#pragma once
#include "./primary.hh"
#include "./type.hh"
#include "../../GenMacro.h"
#include "./reference.hh"
#include "./literal.hh"
#include "./allocating_expr.hh"
enum UnaryOpType
{
    UNARY_ENUM_MEMBERS
};

enum BinaryOpType
{
    INFIX_ENUM_MEMBERS
};

static std::map<TokenType, UnaryOpType> prefix_op_mappings{PREFIX_ENUM_MAPPINGS};
static std::map<TokenType, UnaryOpType> postfix_op_mappings{POSTFIX_ENUM_MAPPINGS};
static std::map<TokenType, BinaryOpType> binary_op_mappings{INFIX_ENUM_MAPPINGS};

class UnaryOperation : public Expr
{
public:
    std::unique_ptr<Expr> operand;
    UnaryOpType op_type;
    std::string str()
    {
        return "Some Unary op";
    }

    UnaryOperation(std::unique_ptr<Expr> operand, UnaryOpType kind)
    {
        this->operand = std::move(operand);
        this->op_type = kind;
    }
    std::vector<Statement *> flatten()
    {
        std::vector<Statement *> ret = {this};
        for (auto m : operand->flatten())
            ret.push_back(m);
        return ret;
    }
    llvm::Value *assign_ptr(VariableTable &vars)
    {
        switch (op_type)
        {
        case PRE_star:
            return operand->llvm_value(vars);
        default:
            except(E_BAD_ASSIGNMENT, "Cannot get an assignable reference to unary operation of type " + std::to_string(op_type));
        }
    }
    llvm::Value *llvm_value(VariableTable &vars, llvm::Type *expected)
    {
        auto _bool = Primitive::get(PR_boolean)->llvm_type();

        #define bld (*builder())
        #define val(expected) operand->llvm_value(vars, expected)
        
        #define ret(LL_INST, cast_to) return cast(bld.Create##LL_INST(val(cast_to)), expected);
        switch (op_type)
        {
        case PRE_ampersand:
        {
            auto ptr = operand->assign_ptr(vars);
            return cast(ptr, expected);
        }
        case PRE_bang: ret(Not, _bool);
        case PRE_minus:ret(Neg, nullptr);
        case PRE_bitwise_not:ret(Not, nullptr);
        case PRE_star:
        {
            if(!operand->type()->get<Ptr>())except(E_BAD_OPERAND, "Cannot dereference non-ptr operand");
            auto value = operand->llvm_value(vars);
            return builder()->CreateLoad(value->getType()->getPointerElementType(), value);
        }
        case PRE_increment:
        case PRE_decrement:
        case POST_increment:
        case POST_decrement:
        {
            auto ptr = operand->assign_ptr(vars);

            auto val = operand->llvm_value(vars);
            auto one = cast(bld.getInt32(1), val->getType());
            auto changed = op_type == POST_increment || op_type == POST_decrement ? bld.CreateAdd(val, one) : bld.CreateSub(val, one);

            llvm::Value* return_val = op_type == POST_increment || op_type == POST_decrement ? operand->llvm_value(vars, expected) 
                                    : changed;
            
            bld.CreateStore(changed, ptr);
            return return_val;
        }
        
        default:except(E_INTERNAL, "Failed to generate llvalue for unary operation: " + std::to_string(op_type));
        }
        
    }

protected:
    std::shared_ptr<Type> get_type()
    {
        auto same_t = operand->type();
        if(!same_t)return same_t;
        auto same_ref = ReferenceType::get(operand->type());
        auto pointee_t = same_t->pointee();
        auto bool_t = Primitive::get(PR_boolean);
        switch (op_type)
        {
        case PRE_ampersand:
            return same_ref;
        case PRE_bang:
            return bool_t;
        case PRE_star:
            return pointee_t;
        case PRE_minus:
            return same_t;
        case PRE_bitwise_not:
            return same_t;
        case PRE_increment:
            return same_t;
        case PRE_decrement:
            return same_t;
        case POST_increment:
            return same_t;
        case POST_decrement:
            return same_t;
        default:
            except(E_INTERNAL, "Failed to get return type of unary operation: " + std::to_string(op_type));
        }
    }
};

class BinaryOperation : public Expr
{
public:
    std::unique_ptr<Expr> left_operand;
    std::unique_ptr<Expr> right_operand;
    BinaryOpType op_type;

    // The name of the variable that this expression
    // is responsible for initializing (if any)
    bool initializes;

    BinaryOperation(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, BinaryOpType op_type)
    {
        left_operand = std::move(left);
        right_operand = std::move(right);
        this->op_type = op_type;
    }
    std::string str()
    {
        auto symbol = this->get_symbol_as_str();
        return "( " + left_operand->str() + " " + symbol + " " + right_operand->str() + " )";
    }
    llvm::Value *assign_ptr(VariableTable &vars)
    {
        except(E_BAD_ASSIGNMENT, "Binary Operations are not assignable");
    }
    llvm::Value *llvm_value(VariableTable &vars, llvm::Type *expected_type = nullptr)
    {
        llvm::Value* value = nullptr;
        if (op_type == BIN_assignment)
        {
            auto assignee = left_operand->assign_ptr(vars);
            // constant assignment check
            if (auto var = dynamic_cast<SourceVariable *>(left_operand.get()))
            {
                auto &va = vars[var->name->str()];
                if (this->initializes && !va.is_initialized)
                {
                    va.is_initialized = true;
                }
                else
                {
                    if (va.is_initialized && va.constant)
                        except(E_BAD_ASSIGNMENT, "Cannot reassign a constant variable: " + var->name->str());
                }
            }

            if(auto literal = dynamic_cast<ArrayLiteral*>(right_operand.get())){
                literal->write_to(assignee, vars);
                // no need to set the value, as the preprocessor enforces that array literals may only
                // be used during initialization
            }
            else{
                auto assignee_assigned_type = assignee->getType()->getPointerElementType();

                if(auto allocating_expr = dynamic_cast<AllocatingExpr*>(right_operand.get())){
                    allocating_expr->write_direct(assignee, vars, assignee_assigned_type);
                }
                else{
                    value = right_operand->llvm_value(vars, assignee_assigned_type);
                    builder()->CreateStore(value, assignee);
                }

            }

            return cast(value, expected_type);
        }




        auto left_val = left_operand->llvm_value(vars);
        auto right_val = right_operand->llvm_value(vars);
        left_val->print(llvm::outs());
        right_val->print(llvm::outs());
        auto op = get_op(left_val, right_val);
        op->print(llvm::outs());
        expected_type->print(llvm::outs());
        auto ret = cast(op, expected_type);
        ret->print(llvm::outs());
        return ret;
    }

    std::vector<Statement *> flatten()
    {
        std::vector<Statement *> ret = {this};
        for (auto m : left_operand->flatten())
            ret.push_back(m);
        for (auto m : right_operand->flatten())
            ret.push_back(m);
        return ret;
    }

private:
    std::string get_symbol_as_str(){
        #define X(t, sym)case BIN_##t:return #sym;
        switch (this->op_type) {
            X(plus, +)
            X(minus, -)
            X(star, *)
            X(slash, /)
            X(percent, %)
            X(lesser, <)
            X(greater, >)
            X(greater_eq, >=)
            X(lesser_eq, <=)
            X(equals, ==)
            X(not_equals, !=)
            default: return "op";
        }
        #undef X
    }
    llvm::Value *get_op(llvm::Value *l, llvm::Value *r)
    {
#define X(myop, opname) \
    case BIN_##myop:    \
        return builder()->Create##opname(l, r);
        switch (op_type)
        {
            X(plus, Add)
            X(minus, Sub)
            X(star, Mul)
            X(slash, SDiv)
            X(percent, SRem)
            X(lesser, ICmpSLT)
            X(greater, ICmpSGT)
            X(lesser_eq, ICmpSLE)
            X(greater_eq, ICmpSGE)
            X(not_equals, ICmpNE)
            X(equals, ICmpEQ)
            X(bitwise_or, Or)
            X(bitwise_and, And)
            X(bitwise_shl, Shl)
            X(bitwise_shr, AShr)
            X(bitwise_xor, Xor)
            X(bool_and, LogicalAnd)
            X(bool_or, LogicalOr)
        default:
            except(E_INTERNAL, "Failed to generate binary operation");
        }
#undef X
    }

protected:
    std::shared_ptr<Type> get_type()
    {
        auto boo = Primitive::get(PR_boolean);
        switch (op_type)
        {
            case BIN_bool_and:
            case BIN_bool_or:
            case BIN_greater:
            case BIN_greater_eq:
            case BIN_equals:
            case BIN_not_equals:
            case BIN_lesser:
            case BIN_lesser_eq:
            return boo;

            case BIN_plus:
            case BIN_minus:
            case BIN_star:
            case BIN_slash:
            case BIN_percent:
            case BIN_bitwise_or:
            case BIN_bitwise_and:
            case BIN_bitwise_shl:
            case BIN_bitwise_shr:
            case BIN_bitwise_xor:
                return left_operand->type();
            case BIN_assignment:
                return right_operand->type();

            default:
                except(E_INTERNAL, "Failed to get type for op: " + std::to_string(op_type));
            }
    }
};

class MemberAccess : public Expr{
public:
    std::unique_ptr<Expr> member_of;
    std::unique_ptr<Name> member_name;

    MemberAccess(std::unique_ptr<Expr> member_of, std::unique_ptr<Name> member_name)
    {
        this->member_of = std::move(member_of);
        this->member_name = std::move(member_name);
    }
    std::string str()
    {
        return member_of->str() + "." + member_name->str();
    }
    llvm::Value *assign_ptr(VariableTable &vars)
    {
        auto strct = member_of->assign_ptr(vars);
        auto strct_t = member_of->type()->get<StructType>();
        if(!strct_t)except(E_BAD_MEMBER_ACCESS, "Cannot access members of non-struct");

        auto idx = strct_t->member_idx(member_name->str());

        auto ptr = builder()->CreateStructGEP(strct_t->llvm_type(), strct, idx);
        return ptr;
    }
    llvm::Value *llvm_value(VariableTable &vars, llvm::Type *expected_type = nullptr) {
        if(this->member_of->type()->get<DynListType>()){
            auto slice_val = member_of->assign_ptr(vars);
            if(member_name->str() == "len"){
                auto len_ptr = builder()->CreateStructGEP(slice_val->getType()->getPointerElementType(), slice_val, 0);
                auto len = builder()->CreateLoad(len_ptr->getType()->getPointerElementType(), len_ptr);
                return cast(len, expected_type);
            }
            else if(member_name->str() == "ptr"){
                auto ref_ptr = builder()->CreateStructGEP(slice_val->getType()->getPointerElementType(), slice_val, 1);
                auto ref = builder()->CreateLoad(ref_ptr->getType()->getPointerElementType(), ref_ptr);
                return cast(ref, expected_type);
            }
            else except(E_BAD_MEMBER_ACCESS, "slices only have the 'ptr' and 'len' properties");
        }
        auto ptr = this->assign_ptr(vars);
        return builder()->CreateLoad(this->get_type()->llvm_type(), ptr);
    }
    std::vector<Statement *> flatten()
    {
        std::vector<Statement *> ret = {this};
        for (auto m : member_of->flatten())
            ret.push_back(m);
        return ret;
    }
protected:
    std::shared_ptr<Type> get_type(){
        auto left_t = member_of->type();
        if(!left_t)return std::shared_ptr<Type>(nullptr);
        auto member = member_name->str();
        if(auto strct = left_t->get<StructType>()){
            if(strct->member_idx(member) == -1)except(E_BAD_MEMBER_ACCESS, "The struct: " + member_of->str() + " does not have a member: " + member);
            return strct->members[member];
        }
        else if(auto slice = left_t->get<DynListType>()){
            if(member == "len"){
                return Primitive::get(PR_int64);
            }
            else if(member == "ptr"){
                return Ptr::get(slice->of);
            }
            else except(E_BAD_MEMBER_ACCESS, "slices only have the properties 'len' and 'ptr'");
        }

        except(E_BAD_MEMBER_ACCESS, "You may only access members of a container type (structs, slices, arrays)");


    }
};