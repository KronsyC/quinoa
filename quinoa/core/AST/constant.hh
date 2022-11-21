/**
 * Constant
 *
 * constants include all primitive data types such as:
 * Integers
 * Floats
 * Strings
 */

#pragma once
#include "./primary.hh"
#include "./include.hh"
#include "./type.hh"
#include "./allocating_expr.hh"
class ConstantValue : public Expr{
public:
    virtual llvm::Constant* const_value(llvm::Type* expected) = 0;

};

template <typename T, typename U>
class Constant : public ConstantValue
{
public:
    T value;
    Constant(T value)
    {
        this->value = value;
    }

    static std::unique_ptr<U> get(T value)
    {
        auto pt = std::make_unique<U>(value);
        return pt;
    }
    std::vector<Statement*> flatten(){
        return {this};
    }

    llvm::Value* assign_ptr(VariableTable& vars){
        except(E_BAD_ASSIGNMENT, "Constant values are not assignable");
    }

};
class String : public AllocatingExpr, public Constant<std::string, String>
{
public:
    using Constant::Constant;
    using AllocatingExpr::llvm_value;
    std::vector<Statement*> flatten(){
        return Constant::flatten();
    }
    llvm::Value* assign_ptr(VariableTable& vars){
        return Constant::assign_ptr(vars);
    }
    std::string str(){
        return "\""+value+"\"";
    }
    llvm::Value *llvm_value(VariableTable& vars, llvm::Type* expected){
        return AllocatingExpr::llvm_value(vars, expected);
    }
    void write_direct(llvm::Value* alloc, VariableTable& vars, llvm::Type* expected){
        auto mod = builder()->GetInsertBlock()->getModule();

        // Generate the string bytes
        std::vector<llvm::Constant*> chars;
        for(auto _char : value){
            chars.push_back(builder()->getInt8(_char));
        }

        auto global_str_bytes_ty = llvm::ArrayType::get(builder()->getInt8Ty(), chars.size());
        auto global_str_initializer = llvm::ConstantArray::get(global_str_bytes_ty, chars);
        auto global_str_var = new llvm::GlobalVariable(*mod, global_str_bytes_ty, true, llvm::GlobalValue::LinkageTypes::PrivateLinkage, global_str_initializer, ".str");
        auto len = builder()->getInt64(chars.size());
        auto cast_arr = builder()->CreateBitCast(global_str_var, llvm::ArrayType::get(builder()->getInt8Ty(), 0)->getPointerTo());

        auto len_ptr = builder()->CreateStructGEP(alloc->getType()->getPointerElementType(), alloc, 0);
        auto arr_ptr = builder()->CreateStructGEP(alloc->getType()->getPointerElementType(), alloc, 1);

        builder()->CreateStore(len, len_ptr);
        builder()->CreateStore(cast_arr, arr_ptr);
    }
    std::shared_ptr<Type> get_type(){
        return DynListType::get(Primitive::get(PR_int8));
    }
    llvm::Constant* const_value(llvm::Type* expected){


        except(E_INTERNAL, "Constant Strings are illegal");
    }
};

class Integer : public Constant<unsigned long long, Integer>
{
public:
    using Constant::Constant;
    std::string str()
    {
        return std::to_string(value);
    }

    llvm::Constant* const_value(llvm::Type* expected){

        auto val = builder()->getInt64(value);
        if(!expected){
            return val;
        }
        auto cast = llvm::ConstantExpr::getIntegerCast(val, expected, true);
        return cast;

    }
    llvm::Value* llvm_value(VariableTable& vars, llvm::Type* expected){
        return const_value(expected);
    }

protected:
    std::shared_ptr<Type> get_type()
    {
        if (value <= maxVal(8))
            return Primitive::get(PR_int8);
        if (value <= maxVal(16))
            return Primitive::get(PR_int16);
        if (value <= maxVal(32))
            return Primitive::get(PR_int32);
        if (value <= maxVal(64))
            return Primitive::get(PR_int64);
        except(E_BAD_EXPRESSION, "Cannot infer type for integer larger than 64 bits");
    }

private:
    inline unsigned long long maxVal(int bits)
    {
        // because of the way the max is found, anything past
        // 63 bits will yield 0, hence this has to be manually
        // returned
        if (bits > 64)
            except(E_INTERNAL, "Cannot Get the Max Val larger than 64 bits");
        if (bits == 64)
            return 18446744073709551615U;
        return ((long long)1 << bits) - 1;
    }
};

