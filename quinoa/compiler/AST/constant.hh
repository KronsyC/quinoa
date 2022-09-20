#pragma once
#include "./ast.hh"
#include<string>

template<typename T>
class Constant:public Expression{
public:
    T value;
    Constant(T value){
        this->value = value;
    }
};
class Integer:public Constant<long long>{
    using Constant::Constant;
};
class Float: public Constant<long double>{
    using Constant::Constant;
};
class String:public Constant<std::string>{
    using Constant::Constant;
};
class Boolean:public Constant<bool>{
    using Constant::Constant;
};
