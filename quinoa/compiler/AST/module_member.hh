#pragma once
#include "./ast.hh"

class Param : public AstNode{

public:
    Type* type = nullptr;
    Ident* name = nullptr;
    bool isVariadic = false;

    Param* deVarify(){
        auto list = type->list();
        if(!list)error("Cannot have non-list varargs: " + name->str());
        auto p = new Param(list->elements, name);
        return p;
    }
    Param(Type* type, Ident* name){
        this->type = type;
        this->name = name;
    }
};

class MethodSigStr{
public:
    MethodSigStr() = default;
    public:
    ModuleMemberRef* name;
    Block<Param> params;
    Block<Generic> generics;
    bool nomangle = false;


    std::string str(){
        if(nomangle)return name->str();
        std::string n = "fn_";
        n+=name->str();
        if(generics.size()){
            n+="<";
            bool first = true;
            for(auto g:generics){
                if(!first)n+=",";
                n+=g->str();
                first = false;
            }
            n+=">";
        }
        if(params.size()){
            n+="(";
            bool first = true;
            for(auto p:params){
                if(!first)n+=",";
                if(p->isVariadic)n+="...";
                n+=p->type->str();
                first = false;
            }
            n+=")";
        }
        return n;
    }
    bool isVariadic(){
        if(params.size()==0)return false;
        return params[params.size()-1]->isVariadic;
    }
    Param* getParam(unsigned int n){
        if(isVariadic()){
            if(n>=params.size()-1)return params[params.size()-1]->deVarify();
            else return params[n];
        }
        else{
            if(n > params.size()-1)return nullptr;
            return params[n];            
        }
    }
};


class TopLevelMetadata{
public:
    Block<Expression> parameters;
    std::string name;
};


class Property:public ModuleMember{

};

class Method;
// Method definitions hold all the info of a method required to call
// and generate definitions for it
class MethodSignature:public AstNode{
public:
    ModuleMemberRef* name;
    bool nomangle = false;
    Block<Param> params;
    Block<Generic> generics;
    Type* returnType = nullptr;
    Method* belongsTo = nullptr;
    bool assured_generic = false;
    bool isGeneric(){
        return assured_generic || generics.size() > 0;
    }
    std::string sourcename(){
        std::string ret;


        auto sigs = sigstr();
        ret+=sigs.str();

        return ret;
    }
    
    std::pair<Block<Param>, Block<Generic>> cloneGenericsParams(){
        Block<Generic> gs;
        Block<Param> ps;
        std::map<std::string, Generic*> genericMappings;
        for(auto g:generics){
            auto gen = new Generic(*g);
            genericMappings[gen->str()] = gen;
            gs.push_back(gen);
        }
        for(auto p:params){
            auto param = new Param(*p);
            param->type = p->type->copy(nullptr);
            auto t = param->type;
            if(auto ref = t->custom()){
                for(auto pair:genericMappings){
                    if(ref->str() == pair.first){
                        ref->refersTo = pair.second;
                        break ;
                    }
                }
            }
            ps.push_back(param);
        }
        return {ps.take(), gs.take()};
    }
    MethodSigStr sigstr(){
        MethodSigStr sigs;
        sigs.name = name;

        sigs.nomangle = nomangle;
        // copy the generics, so we can mess with them
        auto pair = cloneGenericsParams();
        sigs.params = pair.first;
        sigs.generics = pair.second;

        return sigs;
    }
    Param* getParam(int n){
        return sigstr().getParam(n);
    }
    bool isVariadic(){
        return sigstr().isVariadic();
    }

    MethodSignature* impl_as_generic(Block<Generic> replaceWith){
        auto newsig = new MethodSignature(*this);
        if(replaceWith.size() != generics.size())error("Cannot generate a replacement signature if generic lengths do not match");
        for(auto g:replaceWith){
            if(!g->refersTo)error("Cannot clone a method signature with unresolved generics");
        }
        auto gp = cloneGenericsParams();
        newsig->generics = gp.second;
        newsig->params = gp.first;

        int i = 0;
        for(auto g:newsig->generics){
            g->refersTo = replaceWith[i]->refersTo;
            i++;
        }
        newsig->assured_generic = true;
        // Replace the parameters
        return newsig;
    }

    MethodSignature* copy(){
        auto sig = new MethodSignature;
        sig->name = name->copy(nullptr);
        sig->nomangle = nomangle;
        sig->returnType = returnType->copy(nullptr);
        sig->belongsTo = belongsTo;
        sig->assured_generic = assured_generic;
        return sig;
    }
};
class MethodPredeclaration:public TopLevelExpression{
public:
    MethodSignature* sig;
    MethodPredeclaration(MethodSignature* sig){
        this->sig = sig;
    }
};
class Method:public ModuleMember, public SourceBlock{
public:
    Method* copy(SourceBlock* ctx){
        auto m = new Method;
        for(auto el:*this){
            m->push_back(el->copy(m));
        }
        m->sig = sig->copy();
        m->ctx = ctx;
        m->local_types = new LocalTypeTable;
        *m->local_types = *local_types;
        return m;
    }
    bool generate(){
        if(auto mod = this->memberOf){
            auto size = mod->generics.size();
            if(size)
            return false;
        }
        if(!sig)error("Method got no sig");
        return !sig->isGeneric();
    }
    Module* memberOf = nullptr;
    MethodSignature* sig = nullptr;
    // If nomangle is enabled, matching is done via names directly
    // instead of using the overload chosing algorithm
    std::vector<Type*> paramTypes(){
        std::vector<Type*> ret;
        for(auto p:sig->params){
            ret.push_back(p->type);
        }
        return ret;
    }
    MethodSigStr sigstr(){
        return sig->sigstr();
    }
    Identifier* fullname(){
        return sig->name;
    }
    void genFor(MethodSignature* sig){
        if(generate())error("Cannot generate non-generic function");
        auto m = this->copy(nullptr);
        m->sig = sig;
        sig->generics.clear();
        sig->assured_generic = false;

        std::map<std::string, Type*> param_types;
        for(auto p:sig->params){
            param_types[p->name->str()] = p->type;
        }

        // make the appropriate modifications to the type table
        for(auto kv:*m->local_types){
            auto varname = kv.first;
            for(auto kv:param_types){
                auto pname = kv.first;
                auto pt = kv.second;

                if(pname == varname){
                    (*m->local_types)[varname] = pt;
                    break;
                }
            }
        }
        
        // Reroot the SourceBlock tree to the new method
        
        this->memberOf->push_back(m);
    }
};
class Entrypoint:public TopLevelExpression, public Method{
public:
    MethodSignature* calls;
    Entrypoint(MethodSignature* calls){
        this->calls = calls;
    }

};