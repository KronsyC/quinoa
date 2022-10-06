#pragma once
#include "./ast.hh"
#include <string>

template <typename T>
class Constant : public Expression
{
public:
    T value;
    Constant(T value)
    {
        this->value = value;
    }
    std::vector<Statement*> flatten(){
        return {this};
    }
};
class Integer : public Constant<long long>
{
    using Constant::Constant;
    inline long long maxVal(int bits){
        return (1 << bits) -1;
    }   
    Type *getType(LocalTypeTable _)
    {
        if(value <= maxVal(8))return new Primitive(PR_int8);
        if(value <= maxVal(16))return new Primitive(PR_int16);
        if(value <= maxVal(32))return new Primitive(PR_int32);
        if(value <= maxVal(64))return new Primitive(PR_int64);
        error("Cannot infer types for ints larger than 64 bit");
        return new Primitive(PR_int64);
    }

};
class Float : public Constant<long double>
{
    using Constant::Constant;
    Type *getType()
    {
        return new Primitive(PR_float64);
    }
};
class String : public Constant<std::string>
{
    using Constant::Constant;
    Type *getType()
    {
        return new TPtr(new Primitive(PR_int8));
    }
};
class Boolean : public Constant<bool>
{
    using Constant::Constant;
    Type *getType()
    {
        return new Primitive(PR_boolean);
    }
};
