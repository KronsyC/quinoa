#pragma once
#include "./ast.hh"
#include <string>
#include <cmath>
class Value : public Expression
{
};
class Constant:public Value{
public:
    Constant* constant(){
        return this;
    }
    virtual llvm::Constant* getLLConstValue(llvm::Type* expected){
        error("Cannot get the ConstantValue of the base constant class");
        return nullptr;
    }
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
    ConstantValue* copy(SourceBlock* _){
        return this;
    }
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
        return this->getLLConstValue(expected);
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
    llvm::Constant *getLLConstValue(llvm::Type *expected = nullptr)
    {
        auto myType = getType();
        auto myVal = builder()->getIntN(myType->getLLType()->getPrimitiveSizeInBits(), value);
        if (!expected)
            return myVal;
        else{
            auto opcode = llvm::CastInst::getCastOpcode(myVal, true, expected, true);
            auto cast =  llvm::ConstantExpr::getCast(opcode, myVal, expected);
            return cast;

        }
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
        return new TPtr(Primitive::get(PR_int8));
    }
    llvm::Constant *getLLConstValue(llvm::Type *expected = nullptr)
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
    llvm::Constant *getLLConstValue(llvm::Type *expected = nullptr)
    {
        auto val = value ? builder()->getTrue() : builder()->getFalse();
        if (!expected)
            return val;
        else{
            auto opcode = llvm::CastInst::getCastOpcode(val, true, expected, true);
            return llvm::ConstantExpr::get(opcode, val, 0, expected);
        }
    }
};

class List : public Value, public Block<Expression>
{
private:

    Type* elementType;

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
    void setElementsType(Type* type){
        this->elementType = type;
    }
    Type *getElementsType()
    {
        // if there is a set type, use that
        if(elementType)return elementType;
        // otherwise, infer the type from the members
        return getCommonType(*this);
    }
    Type *getType()
    {
        // get the common type for each pair of elements
        // and propogate upwards to one common type
        // put this within a list and return it
        return new ListType(getElementsType());
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
        auto arrType = llvm::ArrayType::get(getElementsType()->getLLType(), members.size());
        if (isStatic())
        {
            auto mod = builder()->GetInsertBlock()->getModule();
            auto initialValues = *(std::vector<llvm::Constant*>*)&members;
            auto initializer = llvm::ConstantArray::get(arrType, initialValues);
            auto arr = new llvm::GlobalVariable(*mod, arrType, true, llvm::GlobalValue::InternalLinkage, initializer, "constant_array");
            return arr;
            // list = llvm::ConstantArray::get(constType, const_members);
        }
        else{
            // attempt to emulate a global value's access pattern to
            // simplify code
            auto alloc = builder()->CreateAlloca(arrType, nullptr, "literal_list");

            // Write each member of the alloc
            int i = 0;
            for(auto item:*this){
                auto val = cast(item->getLLValue(vars), getElementsType()->getLLType());
                auto gep = builder()->CreateConstInBoundsGEP2_64(arrType, alloc, 0, i);
                builder()->CreateStore(val, gep);
                i++;
            }
            return alloc;
        }
        return list;
    }
};