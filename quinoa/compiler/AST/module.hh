#pragma once
#include "./ast.hh"
class Import:public TopLevelExpression{
public:
    CompoundIdentifier* target;
    bool isStdLib = false;
    CompoundIdentifier* alias;

    Import(CompoundIdentifier* tgt, bool std, CompoundIdentifier* alias){
        target = tgt;
        isStdLib = std;
        this->alias = alias;
    }
    bool isImport(){
        return true;
    }
};
class ModuleReference:public Block<Expression>{
public:
    Identifier* name;
};

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
    CompoundIdentifier* space;
    MethodSigStr() = default;
    public:
    Ident* name;
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
    Param* getParam(int n){
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



// Method definitions hold all the info of a method required to call
// and generate definitions for it
class MethodSignature:public AstNode{
public:
    Ident* name = nullptr;
    bool nomangle = false;
    CompoundIdentifier* space = nullptr;
    Block<Param> params;
    Block<Generic> generics;
    Type* returnType = nullptr;

    CompoundIdentifier* fullname(){

        if(!space)return new CompoundIdentifier({name});
        return  new CompoundIdentifier({space, name});
    }
    bool isGeneric(){
        return generics.size() > 0;
    }
    std::string sourcename(){
        std::string ret;
        if(space){
            ret+=space->str()+".";
        }
        auto sigs = sigstr();
        ret+=sigs.str();

        return ret;
    }
    MethodSigStr sigstr(){
        MethodSigStr sigs;
        sigs.name = name;
        sigs.space = space;
        sigs.nomangle = nomangle;
        // copy the generics, so we can mess with them
        std::map<std::string, Generic*> genericMappings;
        for(auto g:generics){
            auto gen = new Generic(*g);
            genericMappings[gen->str()] = gen;
            sigs.generics.push_back(gen);
        }
        for(auto p:params){
            auto param = new Param(*p);
            param->type = p->type->copy();
            auto t = param->type;
            if(auto ref = t->custom()){
                for(auto pair:genericMappings){
                    if(ref->str() == pair.first){
                        ref->refersTo = pair.second;
                        break ;
                    }
                }
            }
            sigs.params.push_back(param);
        }

        if(space==nullptr && !nomangle)error("Space is null?", true);
        return sigs;
    }
    Param* getParam(int n){
        return sigstr().getParam(n);
    }
    bool isVariadic(){
        return sigstr().isVariadic();
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
    bool generate = true;
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
    CompoundIdentifier* fullname(){
        return sig->fullname();
    }

};
class Entrypoint:public TopLevelExpression, public Method{
public:
    MethodSignature* calls;
    Entrypoint(MethodSignature* calls){
        this->calls = calls;
    }

};
class Module:public TopLevelExpression, public Block<ModuleMember>{
public:
    CompoundIdentifier* name;
    std::vector<ModuleReference*> compositors; 
    bool isModule(){
        return true;
    }

    bool is(std::string comp){
        for(auto c:compositors){
            if(c->name->str() == comp)return true;
        }
        return false;
    } 
    void remove(std::string comp){
        int idx = -1;
        for(int i  = 0;i<compositors.size();i++){
            auto c = compositors[i];
            if(c->name->str() == comp){
                idx = i;
                break;
            }
        }
        if(idx!=-1){
            
            compositors.erase(compositors.begin()+idx);
        }
    }
    Method* getMethod(std::string name){
        for(auto m:*this){
            auto mt = (Method*)m;
            if(instanceof<Method>(m) && mt->sig->name->str() == name)return mt;

        }
        return nullptr;
    }
    std::vector<Method*> getAllMethods(){
        std::vector<Method*> ret;
        for(auto m:*this){
            auto mt = (Method*)m;
            if(instanceof<Method>(m))ret.push_back(mt);
        }
        return ret;
    }
    bool hasMethod(std::string name){
        return this->getMethod(name) ;
    } 
};



