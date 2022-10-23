#pragma once
#include "./ast.hh"

struct Identifier:public Expression{
public:
    virtual std::string str(){
        error("Cannot get Str of raw identifier");
        return "";
    }
    virtual const char* c_str(){
        return "";
    }
    std::vector<Statement*> flatten(){
        error("Cannot flatten base Identifier type");
        return {this};
    }
    
    virtual llvm::AllocaInst* getPtr(TVars vars){
        error("Cannot getPtr to a base identifier");
        return nullptr;
    }
    Identifier* copy(SourceBlock* ctx){
        error("Cannot copy base identifier type");
        return nullptr;
    }
};




class Ident:public Identifier{
private:
    Ident(std::string name){
        this->name = name;
    }
public:
    Ident() = default;
    Ident* copy(SourceBlock* ctx){
        return get(name, ctx);
    }
    std::string name;
    std::string str(){
        auto name = this->name;
        return name;
    }
    const char* c_str(){
        return std::move(str().c_str());
    }

    Type* getType(){
        if(!ctx)error("No Context for Ident");
        auto type = ctx->getType(str());
        
        if(!type)error("Failed to locate type for " + str());
        return type;
    }
    std::vector<Statement*> flatten(){
        return {this};
    }
    llvm::AllocaInst* getPtr(TVars vars){
        auto loaded = vars[str()];
        
        if (loaded == nullptr)
            error("Failed to read variable '" + str() + "'");
        return loaded;
    }
    llvm::Value* getLLValue(TVars vars, llvm::Type* expected=nullptr){
        auto loaded = getPtr(vars);
        return cast(builder()->CreateLoad(loaded->getType()->getPointerElementType(), loaded), expected);
    }
    static Ident* get(std::string name, SourceBlock* ctx=nullptr){
        auto id =  new Ident(name);
        id->ctx = ctx;
        return id;
    }
};
class CompoundIdentifier:public Ident{
public:
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto p:parts)for(auto m:p->flatten())ret.push_back(m);
        return ret;
    }

    std::vector<Identifier*> parts;
    CompoundIdentifier(std::vector<Identifier*> parts){
        this->parts = parts;
        flatify();
    }
    CompoundIdentifier(std::string value, SourceBlock* ctx = nullptr)
    {
        auto ident = Ident::get(value, ctx);
        this->parts.push_back(ident);
        flatify();
    }
    CompoundIdentifier(){
    };
    inline bool empty(){
        return this->parts.size() == 0;
    }
    CompoundIdentifier* copy(SourceBlock* ctx){
        auto id = new CompoundIdentifier;
        id->ctx = ctx;
        for(auto p:parts){
            id->parts.push_back(p->copy(ctx));
        }
        return id;
    }
    void flatify(){
        std::vector<Identifier*> flattened;
        for(auto p:parts){
            if(instanceof<CompoundIdentifier>(p)){
                auto q = (CompoundIdentifier*)p;
                q->flatify();
                for(auto p:q->parts){
                    flattened.push_back(p);
                }
            }
            else if(instanceof<Ident>(p)){
                flattened.push_back(p);
            }
            else if(instanceof<Identifier>(p)){
                //FIXME: This will probably break
                continue;
            }
            else if(p == nullptr)error("Cannot flatten as a nullptr was found", true);
            else error("Failed to flatten ident");
        }
        this->parts = flattened;
    }
    std::string str(){
        std::string name;
        bool first = true;
        for(auto p:parts){
            if(!first)name+="::";
            name+=p->str();
            first = false;
        }
        return name;
    }
    const char* c_str(){
        auto s = str();
        return std::move(s.c_str());
    }
    Ident* last(){
        flatify();
        if(parts.size() == 0)error("Cannot get last element of 0-length name");
        // guaranteed to be an Ident after flattening
        auto p = (Ident*)parts.at(parts.size()-1);
        return p;
    }
    // the exact opposite of the last function
    // useful for grabbing the namespace from a fully
    // declared member name
    CompoundIdentifier* all_but_last(){
        flatify();
        auto ret = new CompoundIdentifier;
        for(int i  = 0;i<parts.size()-1;i++){
            auto item = (Ident*)(parts[i]);
            ret->parts.push_back(item);
        }
        return ret;
    }

    bool equals(CompoundIdentifier* compare){
        compare->flatify();
        this->flatify();
        if(compare->parts.size() != this->parts.size())return false;
        for(int i = 0;i<compare->parts.size();i++){
            auto mine = (Ident*)this->parts[i];
            auto cmp = (Ident*)compare->parts[i];
            if(mine->name != cmp->name)return false;
        }
        return true;
    }
};


class Module;

class ModuleRef:public Identifier{
public:
    CompoundIdentifier* name;
    Module* refersTo;
    Block<Type> type_params;
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this};
        for(auto m:name->flatten())ret.push_back(m);
        for(auto tp:type_params)for(auto f:tp->flatten())ret.push_back(f);
        return ret;
    }
    std::string str(){
        std::string ret = name->str();
        if(type_params.size()){
            ret+="<";
            bool first = true;
            for(auto t:type_params){
                if(!first)ret+=",";
                ret+=t->str();
                first = false;
            }
            ret+=">";
        }
        return ret;
    }
    ModuleRef* copy(SourceBlock* ctx){
        auto ret = new ModuleRef;
        ret->name = name->copy(ctx);
        ret->refersTo = refersTo;
        for(auto tp:type_params){
            ret->type_params.push_back(tp->copy(ctx));
        }
        return ret;
    }
};
/**
 * 
 * Keep track of a member of a module,
 * with info about the generic params that may be passed
 * 
*/
class ModuleMemberRef:public Identifier{
public:
    ModuleRef* mod;
    Ident* member;
    ModuleMemberRef* copy(SourceBlock* ctx){
        auto ret = new ModuleMemberRef;
        ret->member = member->copy(ctx);
        ret->mod = mod->copy(ctx);
        return ret;
    }
    std::vector<Statement*> flatten(){
        std::vector<Statement*> ret = {this, member};
        for(auto m:mod->flatten())ret.push_back(m);
        return ret;
    }
    std::string str(){
        return mod->str()+"::"+member->str();
    }
};