/**
 * A Reference is essentially anything that refers to anything else
 * this is intentionally a broad definition
*/
#pragma once

#include "./include.hh"
#include "./primary.hh"
#include "../../lib/logger.h"

class Reference : public ANode {

};

class Name : public Reference {
private:
    std::string value;

public:
    Name(std::string value) {
        this->value = value;
    }

    Name() = default;

    operator std::string() const {
        return value;
    }

    std::string str() {
        return value;
    }

    void set_name(std::string val) {
        this->value = val;
    }
};

class LongName : public Reference {
public:
    Vec<Name> parts;
    bool is_valid = false;
    std::string str() {
        std::string ret;
        bool first = true;
        for (auto &p: parts) {
            if (!first)ret += "::";
            ret += p->str();
            first = false;
        }
        return ret;

    }

    Name &last() {
        return parts[parts.len() - 1];
    }
    
    std::unique_ptr<LongName> all_but_last(){
      auto ret = std::make_unique<LongName>();
      ret->is_valid = true;
      for(unsigned i = 0; i < parts.len() - 1; i++){
          ret->parts.push(parts[i]);
      }
      return ret;
    }

    LongName(LongName &copy_from) {
      is_valid = true;
        for (const auto& p: copy_from.parts) {
            parts.push(*p.ptr);
        }
    }
    LongName(Name n){
      parts.push(n);
    }
    LongName(LongName &&) = default;

    LongName &operator=(LongName &&to) = default;

    LongName() = default;

    operator bool(){
      return is_valid;
    }
};

class SourceVariable : public Name, public Expr {
public:
    std::unique_ptr <LongName> name;

    std::vector<Type*> flatten_types(){
      return {};
    }
    SourceVariable(LongName name) {
        this->name = std::make_unique<LongName>(name);
    }

    SourceVariable(std::unique_ptr <LongName> name) {
        this->name = std::move(name);
    }

    SourceVariable(Name name) {
        this->name = std::make_unique<LongName>();
        this->name->parts.push(name);
    }

    std::string str() {
        return name->str();
    }

    LLVMValue llvm_value(VariableTable &vars, LLVMType expected_type = {}) {

        auto ptr = assign_ptr(vars);
        auto value = ptr.load();
        return cast(value, expected_type);
    }

    LLVMValue assign_ptr(VariableTable &vars) {
        auto &var = vars[name->str()];
        if (!var.type)except(E_BAD_VAR, "Variable " + name->str() + " was accessed before initialization");
        return var.as_value();
    }

    _Type get_type() {
        if (!scope)except(E_INTERNAL, "variable " + str() + " has no scope");
        return scope->get_type(name->str());
    }

    std::vector<Statement *> flatten() {
        std::vector< Statement * > ret = {this};
        return ret;
    }
};

class Container;

class ContainerRef : public Reference {
public:
    Container *refers_to = nullptr;
    std::unique_ptr <LongName> name;

    std::string str();
    std::string mangle_str();
};

class ContainerMemberRef : public Reference {
public:
    std::shared_ptr <ContainerRef> container;
    std::unique_ptr <Name> member;
    bool trunc = false;

    std::string str() {
        auto mem_name = member->str();
        if (trunc || !container)return mem_name;
        auto con_name = container->str();
        return con_name + "::" + mem_name;
    }
    std::string mangle_str(){
        auto mem_name = member->str();
        if (trunc || !container)return mem_name;
        auto con_name = container->mangle_str();
        return con_name + "::" + mem_name;
    }
};
