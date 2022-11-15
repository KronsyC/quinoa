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

class Integer : public Constant<unsigned long long, Integer>
{
public:
    using Constant::Constant;
    std::string str()
    {
        return std::to_string(value);
    }
    llvm::Value *llvm_value(VariableTable& vars, llvm::Type* expected)
    {
        return const_value(expected);
    }
    llvm::Constant* const_value(llvm::Type* expected){

        auto val = builder()->getInt64(value);
        if(!expected){
            return val;
        }
        auto cast = llvm::ConstantExpr::getIntegerCast(val, expected, true);
        return cast;

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

class String : public Constant<std::string, String>{
public:
    using Constant::Constant;
    std::string str(){
        return "\""+value+"\"";
    }
    llvm::Constant* llvm_value(VariableTable& vars, llvm::Type* expected){
        return const_value(expected);
    }
    std::shared_ptr<Type> get_type(){
        return Ptr::get(Primitive::get(PR_int8));
    }
    llvm::Constant* const_value(llvm::Type* expected){
        auto val = builder()->CreateGlobalStringPtr(value);
        auto opcode = llvm::CastInst::getCastOpcode(val, true, expected, true);
        return llvm::ConstantExpr::getCast(opcode, val, expected, false);
    }
};