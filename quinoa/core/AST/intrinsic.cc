#include "intrinsic.hh"
#include "../../lib/error.h"


// Override validation rules for scenarios such as optional args


#define MakesA(_for, _type)template<> std::shared_ptr<Type> Intrinsic<_for>::get_type(){return _type;}

#define MakesBool(_for)MakesA(_for, Primitive::get(PR_boolean))

#define Impl(ret_type, _for) template<> ret_type Intrinsic<_for>

//TODO: properly implement this
#define ArgsRule(_for, min_args, min_type_args, max_args, max_type_args) Impl(void, _for)::validate(){ \
    auto call_name = intrinsic_names[_for];                                                                  \
}


#define CodegenRule(_for) \
    template<> LLVMValue Intrinsic<_for>::llvm_value(VariableTable& vars, LLVMType expected)

#define BinaryCodegenRule(_for, content, error_text) \
    CodegenRule(_for){          \
        LLVMValue left = this->args[0].llvm_value(vars); \
        LLVMValue right = this->args[1].llvm_value(vars);\
        LLVMType op_type = left.type;                                             \
        if(left.type != right.type)except(E_BAD_INTRINSIC_CALL, \
        "The operands of the intrinsic call: " + this->name + " must be of the same type" \
        "\n\t\tArguments were found to be of the types " + left.type.qn_type->str() + " and " + right.type.qn_type->str() + " respectively"                     \
        );   \
        content                                      \
        except(E_BAD_INTRINSIC_CALL, error_text);                                               \
    }

#define BinaryAssertPrimitive \
    if(!left.type.qn_type->get<Primitive>())except(E_BAD_INTRINSIC_CALL, "Calls to " + name + " may only use primitive types"); \
    Primitive& prim_ty = *left.type.qn_type->get<Primitive>();

#define Ret(val) \
    return cast(LLVMValue(val, op_type), expected);


BinaryCodegenRule(intr_add, {
    BinaryAssertPrimitive;

    if(prim_ty.is_float())
        Ret(builder()->CreateFAdd(left, right))

    else if(prim_ty.is_integer())
        Ret(builder()->CreateAdd(left, right))

}, "You can only use @add on on float and integer operands");

BinaryCodegenRule(intr_sub, {
    BinaryAssertPrimitive;

    if(prim_ty.is_float())
        Ret(builder()->CreateFSub(left, right))

    else if(prim_ty.is_integer())
        Ret(builder()->CreateSub(left, right))

}, "You can only use @sub on on float and integer operands")

BinaryCodegenRule(intr_mul, {
    BinaryAssertPrimitive;

    if(prim_ty.is_float()){
        Ret(builder()->CreateFMul(left, right));
    }
    else if(prim_ty.is_integer()){
        Ret(builder()->CreateMul(left, right));
    }
}, "You can only use @mul on on float and integer operands")

BinaryCodegenRule(intr_div, {
    BinaryAssertPrimitive;

    if(prim_ty.is_float())
        Ret(builder()->CreateFDiv(left, right))

    else if(prim_ty.is_signed_integer())
        Ret(builder()->CreateSDiv(left, right))

    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateUDiv(left, right))

}, "You can only use @div on on float and integer operands")

BinaryCodegenRule(intr_mod, {
    BinaryAssertPrimitive;

    if(prim_ty.is_float())
        Ret(builder()->CreateFRem(left, right))

    else if(prim_ty.is_signed_integer())
        Ret(builder()->CreateSRem(left, right))

    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateURem(left, right))

}, "You can only use @mod on on float and integer operands")

BinaryCodegenRule(intr_bitwise_and, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateAnd(left, right))
}, "You can only use @bitwise_and on integers")

BinaryCodegenRule(intr_bitwise_xor, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateXor(left, right))
}, "You can only use @bitwise_xor on integers")

BinaryCodegenRule(intr_bitwise_or, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateOr(left, right))
}, "You can only use @bitwise_or on integers")

BinaryCodegenRule(intr_bitwise_shl, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateShl(left, right))

}, "You can only use @bitwise_shl on integers")

BinaryCodegenRule(intr_bitwise_shr, {
    BinaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateAShr(left, right))
    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateLShr(left, right))
}, "You can only use @bitwise_shr on integers")

MakesBool(intr_bool_and)
BinaryCodegenRule(intr_bool_and, {
    BinaryAssertPrimitive;
    if(prim_ty.is_bool())
        Ret(builder()->CreateLogicalAnd(left, right))

}, "You can only use @bool_and on booleans")

MakesBool(intr_bool_or)
BinaryCodegenRule(intr_bool_or, {
    BinaryAssertPrimitive;
    if(prim_ty.is_bool())
        Ret(builder()->CreateLogicalOr(left, right))
}, "You can only use @bool_or on booleans")

BinaryCodegenRule(intr_cmp_eq, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateICmpEQ(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpOEQ(left, right))
}, "You can only use @cmp_eq on on float and integer operands")

BinaryCodegenRule(intr_cmp_neq, {
    BinaryAssertPrimitive;

    if(prim_ty.is_integer())
        Ret(builder()->CreateICmpNE(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpONE(left, right))
}, "You can only use @cmp_neq on on float and integer operands")

BinaryCodegenRule(intr_cmp_greater, {
    BinaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateICmpSGT(left, right))
    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateICmpUGT(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpOGT(left, right))
}, "You can only use @cmp_gt on on float and integer operands")

BinaryCodegenRule(intr_cmp_greater_eq, {
    BinaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateICmpSGE(left, right))
    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateICmpUGE(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpOGE(left, right))
}, "You can only use @cmp_gte on on float and integer operands")

BinaryCodegenRule(intr_cmp_lesser, {
    BinaryAssertPrimitive;

    if(prim_ty.is_signed_integer())
        Ret(builder()->CreateICmpSLT(left, right))
    else if(prim_ty.is_unsigned_integer())
        Ret(builder()->CreateICmpULT(left, right))
    else if(prim_ty.is_float())
        Ret(builder()->CreateFCmpOLT(left, right))
}, "You can only use @cmp_lt on on float and integer operands")

BinaryCodegenRule(intr_cmp_lesser_eq, {
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

    LLVMType variable_type = assignee.type.qn_type->pointee();

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

    auto llvm_value = value.llvm_value(vars);

    builder()->CreateStore(llvm_value, assignee);
    return cast(llvm_value, expected);
}

CodegenRule(intr_size_of){
    except(E_INTERNAL, "size_of not implemented");
}

CodegenRule(intr_make_slice){
    except(E_INTERNAL, "make_slice not implemented");
}

CodegenRule(intr_get_member){
    except(E_INTERNAL, "get_member not implemented");

}