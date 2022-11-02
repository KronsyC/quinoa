#pragma once
#include "./ast.hh"
class Import:public TopLevelExpression{
public:
    CompoundIdentifier* target;
    Ident* member = nullptr;
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



class Compositor:public ModuleRef{
public:
    Block<Type> generic_args;
};

class Method;
class Property;
struct CompilationUnit;

class TLContainer : public TopLevelExpression, public Block<ModuleMember>{
public:
    Block<Generic> generics;
    // The name of the Module, i.e 'Test', 'MyModule'
    Ident* name;

    Block<Compositor> compositors; 

    // The Namespace Unique hash for the module
    // i.e abcdefg
    Ident* nspace = nullptr;
    CompoundIdentifier* fullname(){
        auto id = new CompoundIdentifier();
        if(nspace)id->push_back(nspace);
        id->push_back(name);
        return id;
    }
        Compositor* comp(std::string comp){
        for(auto c:compositors){
            if(c->name->str() == comp)return c;
        }
        return nullptr;
    } 

    std::vector<Method*> getAllMethods(){
        std::vector<Method*> ret;

        // immediate children
        for(auto m:*this){
            auto mt = (Method*)m;
            if(instanceof<Method>(m))ret.push_back(mt);
        }

        return ret;
    }
    std::vector<Property*> getAllProperties(){
        Block<Property> ret(false);
        for(auto child:*this){
            if(instanceof<Property>(child))ret.push_back((Property*)child);
        }
        return ret;
    }
};

class Seed: public TLContainer{

};
class Module:public TLContainer{
public:

    llvm::Module* llmod = nullptr;
    llvm::StructType* struct_type = nullptr;

    
    ModuleRef* get_ref(){
        auto ref = new ModuleRef;
        ref->refersTo = this;
        ref->name = this->fullname();
        return ref;
    }
    bool isModule(){
        return true;
    }
};
