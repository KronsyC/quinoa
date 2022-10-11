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
    Param(Type* type, Ident* name){
        this->type = type;
        this->name = name;
    }
    Param* deVarify(){
        if(!instanceof<ListType>(type))error("Cannot have non-list varargs");
        auto list = (ListType*)type;
        auto p = new Param(list->elements, name);
        return p;
    }
};
class MethodSigStr{
public:
    Ident* name;
    std::vector<Param*> params;
    bool nomangle = false;
    std::string str(){
        if(nomangle)return name->str();
        std::string n = "fn_";
        n+=name->str();
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

class QualifiedMethodSigStr:public MethodSigStr{
public:
    CompoundIdentifier* space;
    QualifiedMethodSigStr() = default;
};

// Method definitions hold all the info of a method required to call
// and generate definitions for it
class MethodSignature:public AstNode{
public:
    Ident* name = nullptr;
    bool nomangle = false;
    CompoundIdentifier* space = nullptr;
    std::vector<Param*> params;
    Type* returnType = nullptr;

    CompoundIdentifier* fullname(){

        if(space==nullptr)return new CompoundIdentifier({name});
        return  new CompoundIdentifier({space, name});
    }

    std::string sourcename(){
        std::string ret;
        if(space!=nullptr){
            ret+=space->str()+".";
        }
        ret+=sigstr().str();
        return ret;
    }
    QualifiedMethodSigStr sigstr(){
        QualifiedMethodSigStr sigs;
        sigs.name = name;
        sigs.params = params;
        sigs.space = space;
        sigs.nomangle = nomangle;
        // if(space==nullptr && !nomangle)error("Space is null?", true);
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
    QualifiedMethodSigStr sigstr(){
        return sig->sigstr();
    }
    CompoundIdentifier* fullname(){
        return sig->fullname();
    }
private:
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
        for(auto m:this->items){
            auto mt = (Method*)m;
            if(instanceof<Method>(m) && mt->sig->name->str() == name)return mt;

        }
        return nullptr;
    }
    std::vector<Method*> getAllMethods(){
        std::vector<Method*> ret;
        for(auto m:this->items){
            auto mt = (Method*)m;
            if(instanceof<Method>(m))ret.push_back(mt);
        }
        return ret;
    }
    bool hasMethod(std::string name){
        return this->getMethod(name) != nullptr;
    } 
};



