#pragma once
#include "./ast.hh"
#include <string>
#include <cmath>
template <typename T>
class Constant : public Expression
{
public:
    T value;
    Constant(T value)
    {
        this->value = value;
    }
    Constant() = default;
    std::vector<Statement*> flatten(){
        return {this};
    }
    Type *getType()
    {
        error("GetType not implemented for constant", true);
        return nullptr;
    }
    llvm::Value* getLLValue(TVars vars, llvm::Type* expected=nullptr){
        error("Cannot get llvalue for constant");
        return nullptr;
    }

};
class Integer : public Constant<unsigned long long>
{
public:
    using Constant::Constant;
    inline unsigned long long maxVal(int bits){
        // because of the way the max is found, anything past
        // 63 bits will yield 0, hence this has to be manually
        // returned
        if(bits>64)error("Cannot Get the Max Val larger than 64 bits");
        if(bits==64)return 18446744073709551615U;
        return ((long long)1 << bits)-1;
    }   
    Type *getType()
    {
        if(value <= maxVal(8))return Primitive::get(PR_int8);
        if(value <= maxVal(16))return Primitive::get(PR_int16);
        if(value <= maxVal(32))return Primitive::get(PR_int32);
        if(value <= maxVal(64))return Primitive::get(PR_int64);
        error("Cannot infer types for ints larger than 64 bit");
        return Primitive::get(PR_int64);
    }
    llvm::Value* getLLValue(TVars vars, llvm::Type* expected=nullptr){
        auto myType = getType();
        auto myVal = builder()->getIntN(myType->getLLType()->getPrimitiveSizeInBits(), value);
        if(expected==nullptr)return myVal;
        else return builder()->CreateIntCast(myVal, expected, true);

    }


};
class Float : public Constant<long double>
{
public:
    using Constant::Constant;
    Type *getType()
    {
        return Primitive::get(PR_float64);
    }
};
class String : public Constant<std::string>
{
public:
    String(std::string val){
        this->value = val;
    }
    String() = default;
    Type *getType()
    {
        return TPtr::get(Primitive::get(PR_int8));
    }
    llvm::Value* getLLValue(TVars vars, llvm::Type* expected=nullptr){
        auto st = builder()->CreateGlobalStringPtr(value, "str");
        Logger::debug("made str");
        return st;
    }
};
class Boolean : public Constant<bool>
{
public:
    using Constant::Constant;
    Type *getType()
    {
        return Primitive::get(PR_boolean);
    }
    virtual llvm::Value* getLLValue(TVars vars, llvm::Type* expected=nullptr){
        auto val = value?builder()->getTrue():builder()->getFalse();
        if(expected==nullptr)return val;
        else return builder()->CreateIntCast(val, expected, true);
    }

};
