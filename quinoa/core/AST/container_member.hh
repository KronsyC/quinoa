/**
 * Container Members are anything that goes inside a container
 * includes:
 * - Properties
 * - Methods 
*/

#pragma once

#include "./reference.hh"
#include "./constant.hh"
#include "type.hh"
#include <string>

class Container;

class Attribute {
public:
    std::string name;
    Vec<ConstantValue> arguments;
};

class ContainerMember : public ANode {
public:
    std::unique_ptr <ContainerMemberRef> name;
    Container *parent;
    Vec<Attribute> attrs;
    bool instance_only = false;
    bool local_only = false;

    bool aliases_resolved = false;

};


class Property : public ContainerMember {
public:
    std::unique_ptr <ConstantValue> initializer;
    std::shared_ptr <Type> type;
};

class TypeMember : public ContainerMember {
public:
    std::shared_ptr <Type> refers_to;
    std::vector<std::shared_ptr<Generic>> generic_args;
    TypeMember(std::shared_ptr <Type> t) {
        this->refers_to = t;
    }
};

class Param : public ANode {
public:
    Name name;
    std::shared_ptr <Type> type;
    bool is_variadic = false;

    Param(Name pname, std::shared_ptr <Type> type)
            : name(pname) {
        this->type = std::move(type);
    }
};


struct MethodSignature{
    std::vector <std::shared_ptr<Generic>> generic_params;
    Vec<Param> parameters;
    std::shared_ptr <Type> return_type;

    bool is_variadic() {
        // Check if the last parameter is a var-arg
        if (!parameters.len())return false;
        auto &last = parameters[parameters.len() - 1];
        return last.is_variadic;
    }


    Param *get_parameter(size_t idx) {
        if (idx < parameters.len())return &parameters[idx];
        else if (is_variadic())return &parameters[parameters.len() - 1];
        else return nullptr;
    }

    bool must_parameterize_return_val(){
        return return_type->get<StructType>() || return_type->get<DynListType>() || return_type->get<ListType>() || return_type->get<ParameterizedTypeRef>();
    }
};


class Method : public MethodSignature, public ContainerMember {
public:
    std::shared_ptr <TypeRef> acts_upon;
    std::vector<std::shared_ptr<Generic>> acts_upon_generic_args;

    std::unique_ptr <Scope> content;

    unsigned application_count = 0;

    void apply_generic_substitution(std::vector<std::shared_ptr<Type>> types, std::vector<std::shared_ptr<Type>> acts_upon_types = {}){
        if(types.size() != generic_params.size() || acts_upon_types.size() != acts_upon_generic_args.size()){
          Logger::debug(std::to_string(types.size()) + " & " + std::to_string(acts_upon_types.size()) + "  vs  " + std::to_string(generic_params.size()) + " & " + std::to_string(acts_upon_generic_args.size())); 
          
      except(E_BAD_SUBSTITUTION, "Cannot substitute generics with mismatched counts\n\t\tIn function: " + this->name->str());
      
        }
        for(unsigned i = 0; i < types.size(); i++){
            Logger::debug(generic_params[i]->name->str() + " | => | " + types[i]->str());
            this->generic_params[i]->temporarily_resolves_to = types[i];
        }
        for(unsigned i = 0; i < acts_upon_types.size(); i++){
          this->acts_upon_generic_args[i]->temporarily_resolves_to = acts_upon_types[i];
        }  
      application_count++;
    }
  GenericTable generics_as_table(){
    GenericTable ret;
    for(auto g : generic_params){
      if(!g->temporarily_resolves_to)except(E_INTERNAL, "Generic does not resolve");
      ret[g->name->str()] = g->temporarily_resolves_to;
    }
    return ret;
  }
  void undo_generic_substitution(){
    for(auto g : generic_params){
      g->temporarily_resolves_to.reset();
    }
    for(auto g : acts_upon_generic_args){
      g->temporarily_resolves_to.reset();
    }
    application_count--;
    if(application_count < 0){
      except(E_INTERNAL, "negative application count");
    }
  }
    // Get the parameter at a specific index
    // this method is smart and accounts for varargs
    // returns `nullptr` if there is no parameter at the given index
    
    bool is_generic(){
      return generic_params.size() || this->acts_upon_generic_args.size();
    }

    std::shared_ptr<Type> get_target_type(){
      if(!acts_upon)return nullptr;
      if(acts_upon->is_generic()){
        std::vector<std::shared_ptr<Type>> resolved_generics;
        for(auto t : acts_upon_generic_args){
          resolved_generics.push_back(t->drill()->self);
        }
        auto ptref = ParameterizedTypeRef::get(acts_upon, resolved_generics)  ;
        return ptref;
      }
      else return acts_upon;
    }

    std::string source_name() {
        if (this->name->trunc)return this->name->str();
        std::string name;
        name += this->name->mangle_str();
        if (this->generic_params.size()) {
            name += "<";
            bool first = true;
            for (auto g: generic_params) {
                if (!first)name += ",";
                name += g->str();
                first = false;
            }
            name += ">";
        }
        for (auto p: parameters) {
            name += "." + p->type->str();
        }
        return name;
    }


    bool is_equivalent_to(Method *method) {

        // basic checks (extremely strict, possibly lax this in the future)
        if (name->member->str() != method->name->member->str())return false;
        if (generic_params.size() != method->generic_params.size())return false;
        if (parameters.len() != method->parameters.len())return false;

        // type-related checks
        if (return_type->distance_from(*method->return_type) < 0)return false;
        for (size_t i = 0; i < parameters.len(); i++) {
            if (parameters[i].type->distance_from(*method->parameters[i].type) < 0)return false;
        }
        return true;
    }
};
