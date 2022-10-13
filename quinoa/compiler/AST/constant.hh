#pragma once
#include "./ast.hh"
#include <string>
#include <cmath>

class Value : public Expression
{
};
class Constant:public Value{

};
template <typename U>
class ConstantValue : public Constant
{
public:
    ConstantValue(U value)
    {
        this->value = value;
    }
    U value;

    std::vector<Statement *> flatten()
    {
        return {this};
    }
    Type *getType()
    {
        error("GetType not implemented for constant", true);
        return nullptr;
    }
    llvm::Value *getLLValue(TVars vars, llvm::Type *expected = nullptr)
    {
        error("Cannot get llvalue for constant");
        return nullptr;
    }
};
 class Integer : public ConstantValue<unsigned long long>
{
    using ConstantValue::ConstantValue;
    inline unsigned long long maxVal(int bits)
    {
        // because of the way the max is found, anything past
        // 63 bits will yield 0, hence this has to be manually
        // returned
        if (bits > 64)
            error("Cannot Get the Max Val larger than 64 bits");
        if (bits == 64)
            return 18446744073709551615U;
        return ((long long)1 << bits) - 1;
    }

    Type *getType()
    {
        if (value <= maxVal(8))
            return Primitive::get(PR_int8);

        if (value <= maxVal(16))
            return Primitive::get(PR_int16);
        if (value <= maxVal(32))
            return Primitive::get(PR_int32);
        if (value <= maxVal(64))
            return Primitive::get(PR_int64);
        error("Cannot infer types for ints larger than 64 bit");
        return Primitive::get(PR_int64);
    }
    llvm::Value *getLLValue(TVars vars, llvm::Type *expected = nullptr)
    {
        auto myType = getType();
        auto myVal = builder()->getIntN(myType->getLLType()->getPrimitiveSizeInBits(), value);
        if (!expected)
            return myVal;
        else
            return builder()->CreateIntCast(myVal, expected, true);
    }
};
class Float : public ConstantValue<long double>
{
public:
    using ConstantValue::ConstantValue;

    Type *getType()
    {
        return Primitive::get(PR_float64);
    }
};
class String : public ConstantValue<std::string>
{
    using ConstantValue::ConstantValue;

    Type *getType()
    {
        return TPtr::get(Primitive::get(PR_int8));
    }
    llvm::Value *getLLValue(TVars vars, llvm::Type *expected = nullptr)
    {
        auto st = builder()->CreateGlobalStringPtr(value, "str");
        return st;
    }
};
class Boolean : public ConstantValue<bool>
{
public:
    using ConstantValue::ConstantValue;

    Type *getType()
    {
        return Primitive::get(PR_boolean);
    }
    virtual llvm::Value *getLLValue(TVars vars, llvm::Type *expected = nullptr)
    {
        auto val = value ? builder()->getTrue() : builder()->getFalse();
        if (!expected)
            return val;
        else
            return builder()->CreateIntCast(val, expected, true);
    }
};

class List : public Value, public Block<Expression>
{
private:
    Type *getTypeOf(std::vector<Expression *> items)
    {
        if (items.size() == 1)
            return items[0]->getType();
        if (items.size() == 0)
            error("Cannot get type of list with 0 elements");

        // Divide and conquer
        auto splitIdx = items.size() / 2;
        auto beg = items.begin();
        auto end = items.end();
        auto left = std::vector<Expression *>(beg, beg + splitIdx);
        auto right = std::vector<Expression *>(beg + splitIdx, end);

        auto left_t = getTypeOf(left);
        auto right_t = getTypeOf(right);

        return getCommonType(left_t, right_t);
    }

public:
    List() = default;
    std::vector<Statement *> flatten()
    {
        std::vector<Statement *> ret = {this};
        for (auto member : *this)
            for (auto m : member->flatten())
                ret.push_back(m);
        return ret;
    }
    Type *getElementsType()
    {
        return getTypeOf(*this);
    }
    Type *getType()
    {
        // get the common type for each pair of elements
        // and propogate upwards to one common type
        // put this within a list and return it
        return ListType::get(getElementsType());
    }
    bool isStatic()
    {
        bool isStatic = true;
        for (auto m : *this)
        {
            if(!instanceof<Constant>(m)){
                isStatic = false;
                break;
            }
        }
        return isStatic;
    }
    llvm::Value *getLLValue(TVars vars, llvm::Type *expected)
    {
        std::vector<llvm::Value *> members;
        for (auto item : *this)
        {
            auto val = item->getLLValue(vars, getElementsType()->getLLType());
            members.push_back(val);
        }
        llvm::Value *list;
        if (isStatic())
        {
            Logger::debug("Static List");
            auto constType = llvm::ArrayType::get(getElementsType()->getLLType(), members.size());
            auto const_members = *(std::vector<llvm::Constant*>*)(&members);
            list = llvm::ConstantArray::get(constType, const_members);
        }
        else error("nonstatic arrays are currently not supported");
        return list;
    }
};