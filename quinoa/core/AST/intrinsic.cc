#include "intrinsic.hh"
#include "./include.hh"
#include "../llvm_utils.h"
#include "./primary.hh"
#include "../../lib/error.h"
#include "llvm/IR/Intrinsics.h"

// Override validation rules for scenarios such as optional args

#define boolean Primitive::get(PR_boolean)

#define MakesA(_for, _type)template<> std::shared_ptr<Type> Intrinsic<_for>::get_type(){return _type;}

#define MakesBool(_for)MakesA(_for, boolean)


#define CodegenRule(_for) \
    template<> LLVMValue Intrinsic<_for>::llvm_value(VariableTable& vars, LLVMType expected)

#define BinaryCodegenRule(_for, content, error_text) \
    CodegenRule(_for){ \
        [[maybe_unused]] \
        auto this_mod = builder()->GetInsertBlock()->getModule(); \
        LLVMValue left = args[0].llvm_value(vars);  \
        LLVMValue right = args[1].llvm_value(vars); \
        LLVMType op_type = type()->llvm_type();     \
        content                                     \
        except(E_BAD_INTRINSIC_CALL, std::string(error_text) + "\n\t\tLeft operand type: " + left.type.qn_type->str() + "; Right operand type: " + right.type.qn_type->str() );   \
    }

#define BinaryCodegenRuleSameT(_for, content, error_text) \
        BinaryCodegenRule(_for, { \
            if(left.type != right.type){ \
                except(E_BAD_INTRINSIC_CALL, \
                "The operands of the intrinsic call: " + this->name + " must be of the same type"  \
                "\n\t\tArguments were found to be of the types " + left.type.qn_type->str() + " and " + right.type.qn_type->str() + " respectively"); \
            } \
            \
            content \
            }, \
            error_text)

#define BinaryAssertPrimitive \
    auto right_t = right.type.qn_type->get<Primitive>();                          \
    auto left_t = left.type.qn_type->get<Primitive>();                          \
    \
    if(!left_t || !right_t)except(E_BAD_INTRINSIC_CALL, "Calls to " + name + " may only use primitive types"); \
    auto& prim_ty = *left_t;



#define Ret(val) \
{ \
    auto llval = LLVMValue(val, op_type); \
    auto result = cast(llval, expected); \
    return result; \
} 




BinaryCodegenRuleSameT(intr_add, {
    BinaryAssertPrimitive

    if(prim_ty.is_float())
        Ret(builder()->CreateFAdd(left, right))

    else if(prim_ty.is_integer())
        Ret(builder()->CreateAdd(left, right))

}, "You can only use @add on on float and integer operands");

BinaryCodegenRuleSameT(intr_sub, {
    BinaryAssertPrimitive;

    if(prim_ty.is_float())
        Ret(builder()->CreateFSub(left, right))

    else if(prim_ty.is_integer())
        Ret(builder()->CreateSub(left, right))

}, "You can only use @sub on on float and integer operands")

BinaryCodegenRuleSameT(intr_mul, {
    BinaryAssertPrimitive;

    if(prim_ty.is_float()){
        Ret(builder()->CreateFMul(left, right));
    }
    else if(prim_ty.is_integer()){
        Ret(builder()->CreateMul(left, right));
    }
}, "You can only use @mul on on float and integer operands")

BinaryCodegenRuleSameT(intr_div, {
    BinaryAssertPrimitive;

    if(prim_ty.is_float())
        Ret(builder()->CreateFDiv(left, right))

    else if(prim_ty.is_signed_integer())
        Ret(builder()->CreateSDiv(left, right))

    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateUDiv(left, right))

}, "You can only use @div on on float and integer operands")

BinaryCodegenRuleSameT(intr_mod, {
    BinaryAssertPrimitive;

    if(prim_ty.is_float())
        Ret(builder()->CreateFRem(left, right))

    else if(prim_ty.is_signed_integer())
        Ret(builder()->CreateSRem(left, right))

    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateURem(left, right))

}, "You can only use @mod on on float and integer operands")

llvm::Value* raise_to_power(LLVMValue target, LLVMValue exponent){

    // assume target and exponent are primitives


    auto targ_t = target.type.qn_type->get<Primitive>();
    auto exp_t = exponent.type.qn_type->get<Primitive>();

    auto mod = builder()->GetInsertBlock()->getModule();

    LLVMValue targ = cast(target, Primitive::get(PR_float64)->llvm_type());
    

    if(exp_t->is_integer()){
        auto powi = llvm::Intrinsic::getDeclaration(mod, llvm::Intrinsic::powi, {targ->getType(), exp_t->llvm_type()});
        return cast_explicit(LLVMValue{builder()->CreateCall(powi, {targ, exponent}), targ.type}, target.type);
    }
    else if(exp_t->is_float()){
        auto pow = llvm::Intrinsic::getDeclaration(mod, llvm::Intrinsic::pow, {targ->getType(), exp_t->llvm_type()});
        return cast_explicit(LLVMValue{builder()->CreateCall(pow, {targ, exponent}), targ.type}, target.type);
    }
    else except(E_BAD_INTRINSIC_CALL, "Only floats and integers may be used as exponents for exponent intrinsics");
}

MakesA(intr_power, this->args[0].type())
BinaryCodegenRule(intr_power, {
   
    Ret(raise_to_power(left, right));

}, "Failed to generate an @pow intrinsic, the exponent must be an integer or float, and the operand must be a float")

BinaryCodegenRule(intr_nth_root, {
    BinaryAssertPrimitive;


    // Raise the operand to the power of 1/exp


    if(right_t->is_float()){
        LLVMValue exp(builder()->CreateFDiv( llvm::ConstantFP::get(right_t->llvm_type(), 1.0), right), right_t->self);
        Ret(raise_to_power(left, exp));
    }
    else if(right_t->is_signed_integer()){
        auto fp = builder()->CreateSIToFP(right, builder()->getDoubleTy());
        LLVMValue exp( builder()->CreateFDiv(llvm::ConstantFP::get(fp->getType(), 1.0), fp), Primitive::get(PR_float64)->self);
        Ret(raise_to_power(left, exp));
    }
    else if(right_t->is_unsigned_integer()){
        auto fp = builder()->CreateUIToFP(right, builder()->getDoubleTy());
        LLVMValue exp(builder()->CreateFDiv(llvm::ConstantFP::get(fp->getType(), 1.0), fp), Primitive::get(PR_float64)->self);
        Ret(raise_to_power(left, exp));
    }
    else except(E_BAD_INTRINSIC_CALL, "The root of the @nth_root intrinsic must be either a float or integer, but was found to be of type: " + right_t->str());



}, "You can only use @nth_root on on float and integer operands")

BinaryCodegenRuleSameT(intr_bitwise_and, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateAnd(left, right))
}, "You can only use @bitwise_and on integers")

BinaryCodegenRuleSameT(intr_bitwise_xor, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateXor(left, right))
}, "You can only use @bitwise_xor on integers")

BinaryCodegenRuleSameT(intr_bitwise_or, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateOr(left, right))
}, "You can only use @bitwise_or on integers")

BinaryCodegenRuleSameT(intr_bitwise_shl, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateShl(left, right))

}, "You can only use @bitwise_shl on integers")

BinaryCodegenRuleSameT(intr_bitwise_shr, {
    BinaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateAShr(left, right))
    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateLShr(left, right))
}, "You can only use @bitwise_shr on integers")

MakesBool(intr_bool_and)
BinaryCodegenRuleSameT(intr_bool_and, {
    BinaryAssertPrimitive;
    if(prim_ty.is_bool())
        Ret(builder()->CreateLogicalAnd(left, right))

}, "You can only use @bool_and on booleans")

MakesBool(intr_bool_or)
BinaryCodegenRuleSameT(intr_bool_or, {
    BinaryAssertPrimitive;
    if(prim_ty.is_bool())
        Ret(builder()->CreateLogicalOr(left, right))
}, "You can only use @bool_or on booleans")

MakesBool(intr_cmp_eq)
BinaryCodegenRuleSameT(intr_cmp_eq, {
    Logger::debug("icmpeq");
    BinaryAssertPrimitive;
    Logger::debug("asssuc");
    if(prim_ty.is_integer())
        Ret(builder()->CreateICmpEQ(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpOEQ(left, right))
}, "You can only use @cmp_eq on on float and integer operands")

MakesBool(intr_cmp_neq)
BinaryCodegenRuleSameT(intr_cmp_neq, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateICmpNE(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpONE(left, right))
}, "You can only use @cmp_neq on on float and integer operands")

MakesBool(intr_cmp_greater)
BinaryCodegenRuleSameT(intr_cmp_greater, {
    BinaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateICmpSGT(left, right))
    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateICmpUGT(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpOGT(left, right))
}, "You can only use @cmp_gt on on float and integer operands")

MakesBool(intr_cmp_greater_eq)
BinaryCodegenRuleSameT(intr_cmp_greater_eq, {
    BinaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateICmpSGE(left, right))
    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateICmpUGE(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpOGE(left, right))
}, "You can only use @cmp_gte on on float and integer operands")

MakesBool(intr_cmp_lesser)
BinaryCodegenRuleSameT(intr_cmp_lesser, {
    BinaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateICmpSLT(left, right))
    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateICmpULT(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpOLT(left, right))
}, "You can only use @cmp_lt on on float and integer operands")

MakesBool(intr_cmp_lesser_eq)
BinaryCodegenRuleSameT(intr_cmp_lesser_eq, {
    BinaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateICmpSLE(left, right))
    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateICmpULE(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpOLE(left, right))
}, "You can only use @cmp_lte on on float and integer operands")


/*
 *
 * Define Unary Intrinsic Operations
 *
 */

#define UnaryCodegenRule(_for, content, error_text) \
    CodegenRule(_for){          \
        LLVMValue operand = this->args[0].llvm_value(vars); \
        LLVMType op_type = operand.type;                                            \
        content                                      \
        except(E_BAD_INTRINSIC_CALL, error_text);                                               \
    }

#define UnaryAssertPrimitive \
    if(!operand.type.qn_type->get<Primitive>())except(E_BAD_INTRINSIC_CALL, "Calls to " + name + " may only use primitive types"); \
    Primitive& prim_ty = *operand.type.qn_type->get<Primitive>();


UnaryCodegenRule(intr_bitwise_not, {
    UnaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateNot(operand))

}, "You can only use @bitwise_not on integer operands")

MakesBool(intr_bool_not)
UnaryCodegenRule(intr_bool_not, {
    UnaryAssertPrimitive;
    if(prim_ty.is_bool())
        Ret(builder()->CreateNot(operand))

}, "You can only use @bool_not on boolean operands")

UnaryCodegenRule(intr_negate, {
    UnaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateNeg(operand))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFNeg(operand))

}, "You can only @negate signed integers and float operands");


UnaryCodegenRule(intr_add_one, {
    UnaryAssertPrimitive;

    if(prim_ty.is_integer()){
        auto one = builder()->getIntN(1, prim_ty.llvm_type()->getPrimitiveSizeInBits());
        Ret(builder()->CreateAdd(operand, one))
    }
    else if(prim_ty.is_float()){
        auto one = llvm::ConstantFP::get(prim_ty.llvm_type(), 1);
        Ret(builder()->CreateFAdd(operand, one))
    }


}, "You may only use @inc on integer and float operands")

UnaryCodegenRule(intr_sub_one, {
    UnaryAssertPrimitive;

    if(prim_ty.is_integer()){
        auto one = builder()->getIntN(1, prim_ty.llvm_type()->getPrimitiveSizeInBits());
        Ret(builder()->CreateSub(operand, one))
    }
    else if(prim_ty.is_float()){
        auto one = llvm::ConstantFP::get(prim_ty.llvm_type(), 1);
        Ret(builder()->CreateFSub(operand, one))
    }
}, "You may only use @dec on integer and float operands")

UnaryCodegenRule(intr_dereference, {

    if(auto ptr = op_type.qn_type->get<Ptr>())
        Ret(builder()->CreateLoad(ptr->of->llvm_type(), operand))

}, "You can only use @deref on a pointer operand")


UnaryCodegenRule(intr_sqrt, {
        UnaryAssertPrimitive;
        auto mod = builder()->GetInsertBlock()->getModule();
        if(prim_ty.is_float()){
            auto sqrt = llvm::Intrinsic::getDeclaration(mod, llvm::Intrinsic::sqrt, {operand.type});
            Ret(builder()->CreateCall(sqrt, {operand}))
        }
        else if(prim_ty.is_signed_integer()){
            // Cast to f64 first
            auto ty = builder()->getDoubleTy();
            auto op = builder()->CreateSIToFP(operand, ty);
            auto sqrt = llvm::Intrinsic::getDeclaration(mod, llvm::Intrinsic::sqrt, {ty});
            Ret(builder()->CreateFPToSI(builder()->CreateCall(sqrt, {op}), prim_ty.llvm_type()))
        }
        else if(prim_ty.is_unsigned_integer()){
            // Cast to f64 first
            auto ty = builder()->getDoubleTy();
            auto op = builder()->CreateUIToFP(operand, ty);
            auto sqrt = llvm::Intrinsic::getDeclaration(mod, llvm::Intrinsic::sqrt, {ty});
            Ret(builder()->CreateFPToUI( builder()->CreateCall(sqrt, {op}), prim_ty.llvm_type()))
        }
}, "You can only use @sqrt on an integer or float operand")

UnaryCodegenRule(intr_pointer_to, {

    // Allocate some stack memory and push the operand into it
    auto alloc = builder()->CreateAlloca(op_type);
    builder()->CreateStore(operand, alloc);

    return cast( LLVMValue(alloc, Ptr::get(op_type.qn_type)->llvm_type()), expected );

}, "unreachable")


/**
* Utility Operations
 * These ones are generally more complex, and cannot be easily macro-generated
*/

CodegenRule(intr_subscript){
    except(E_INTERNAL, "subscript not implemented");
}

CodegenRule(intr_assign){
    auto assignee = args[0].assign_ptr(vars);

    // Ensure we are not assigning to a constant
    if (auto var = dynamic_cast<SourceVariable *>(&args[0])) {
        auto &variable = vars[var->name->str()];
        if(variable.constant){
            if(!includes(flags, std::string(FLAG_CONST_INIT_RIGHTS))){
                except(E_BAD_ASSIGNMENT, "Cannot reassign a value to a constant variable");
            }


        }
        variable.is_initialized = true;
    }

    auto variable_type = assignee.type.qn_type->drill()->pointee()->llvm_type();

    auto& value = args[1];
    // Assert that the variable type and value type are equal

    if(value.type()->llvm_type() != variable_type)
        except(E_BAD_ASSIGNMENT, "Assigned type: " + value.type()->str() +
        " does not match the container type: " + variable_type.qn_type->str()
        );


    // Optimization for heap data structures like arrays and structs
    if (auto allocating_expr = dynamic_cast<AllocatingExpr *>(&value)) {
        allocating_expr->write_direct(assignee, vars, variable_type);
        return allocating_expr->llvm_value(vars, variable_type);
    }
    Logger::debug("Assigning: " + value.str());
    Logger::debug("Type: " + value.type()->str());
    auto llvm_value = value.llvm_value(vars);

    builder()->CreateStore(llvm_value, assignee);
    return cast(llvm_value, expected);
}

MakesA(intr_size_of, Primitive::get(PR_uint64));
CodegenRule(intr_size_of){
    auto type = this->type_args[0];
    auto size = type->llvm_type()->getPrimitiveSizeInBits();
    auto size_bytes = size / 8;

    // if a type is less than 1 byte (booleans) round up to 1 as that is the size it will end up having
    if(size_bytes == 0)size_bytes++;
    return cast(LLVMValue(builder()->getInt64(size_bytes), this->type()), expected);
}

MakesA(intr_make_slice, DynListType::get(this->type_args[0]));
CodegenRule(intr_make_slice){
    auto internal_ptr  = this->args[0].llvm_value(vars, Ptr::get(this->type_args[0])->llvm_type());
    auto element_count = this->args[1].llvm_value(vars, Primitive::get(PR_uint64)->llvm_type());

    auto slice_ty = this->type();
    auto slice_llty = slice_ty->llvm_type();
    auto alloc = builder()->CreateAlloca(slice_llty);

    auto len_ptr = builder()->CreateStructGEP(slice_llty, alloc, 0);
    auto elements_ptr_raw = builder()->CreateStructGEP(slice_llty, alloc, 1);
    auto elements_ptr = builder()->CreateBitCast(elements_ptr_raw, internal_ptr->getType()->getPointerTo());

    builder()->CreateStore(element_count, len_ptr);
    builder()->CreateStore(internal_ptr, elements_ptr);

    auto load = builder()->CreateLoad(slice_llty, alloc);
    return cast(LLVMValue(load, slice_ty), expected);
}

CodegenRule(intr_get_member){
    except(E_INTERNAL, "get_member not implemented");

}
