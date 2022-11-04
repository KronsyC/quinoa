#pragma once
#include "./ast.hh"

struct Identifier : public Expression {
public:
    virtual std::string str()
    {
	except(E_INTERNAL, "Cannot get Str of raw identifier");
	return "";
    }
    virtual const char* c_str()
    {
	return "";
    }
    std::vector<Statement*> flatten()
    {
	except(E_INTERNAL, "Cannot flatten base Identifier type");
	return { this };
    }

    virtual llvm::AllocaInst* getPtr(TVars vars)
    {
	except(E_INTERNAL, "Cannot getPtr to a base identifier");
	return nullptr;
    }
    Identifier* copy(SourceBlock* _ctx)
    {
	except(E_INTERNAL, "Cannot copy base identifier type");
	return nullptr;
    }
};

class Ident : public Identifier
{
private:
    Ident(std::string name)
    {
	this->name = name;
    }

public:
    Ident() = default;
    Ident* copy(SourceBlock* ctx)
    {
	return get(name, ctx);
    }
    std::string name;
    std::string str()
    {
	auto name = this->name;
	return name;
    }
    const char* c_str()
    {
	return std::move(str().c_str());
    }

    std::vector<Statement*> flatten()
    {
	return { this };
    }
    Variable* getVar(TVars vars)
    {
	auto loaded = vars[str()];

	if(loaded == nullptr)
	    except(E_UNDECLARED_VAR, "Failed to read variable '" + str() + "'");
	return loaded;
    }
    llvm::AllocaInst* getPtr(TVars vars)
    {
	return getVar(vars)->value;
    }
    Type* getType()
    {
	if(!ctx)
	    except(E_INTERNAL, "No Context for Ident: " + name);
	auto type = ctx->getType(str());
	if(!type)
	    Logger::error("Failed to get type of '" + name + "'");
	return type;
    }
    bool isConst(TVars vars)
    {
	return getVar(vars)->constant;
    }
    llvm::Value* getLLValue(TVars vars, llvm::Type* expected = nullptr)
    {
	auto loaded = getPtr(vars);
	return cast(builder()->CreateLoad(loaded->getType()->getPointerElementType(), loaded), expected);
    }
    static Ident* get(std::string name, SourceBlock* ctx = nullptr)
    {
	auto id = new Ident(name);
	id->ctx = ctx;
	return id;
    }
};
class CompoundIdentifier : public Ident, public Block<Ident>
{
public:
    std::vector<Statement*> flatten()
    {
	std::vector<Statement*> ret = { this };
	for(auto p : *this)
	    for(auto m : p->flatten())
		ret.push_back(m);
	return ret;
    }
    CompoundIdentifier(std::vector<Ident*> parts)
    {
	for(auto p : *this) {
	    if(p == nullptr)
		except(E_INTERNAL, "Cannot initialize a Compound Identifier with a nullptr");
	    this->push_back(p);
	}
    }
    CompoundIdentifier(std::string value, SourceBlock* ctx = nullptr)
    {
	auto ident = Ident::get(value, ctx);
	this->push_back(ident);
    }
    CompoundIdentifier() = default;
    inline bool empty()
    {
	return this->size() == 0;
    }
    CompoundIdentifier* copy(SourceBlock* ctx)
    {
	auto id = new CompoundIdentifier;
	id->ctx = ctx;
	for(auto p : *this) {
	    if(p == nullptr)
		continue;
	    id->push_back(p->copy(ctx));
	}
	return id;
    }
    std::string str()
    {
	std::string name;
	bool first = true;
	for(auto p : *this) {
	    if(!first)
		name += ".";
	    name += p->str();
	    first = false;
	}
	return name;
    }
    const char* c_str()
    {
	auto s = str();
	return std::move(s.c_str());
    }
    Ident* last()
    {
	if(size() == 0)
	    except(E_INTERNAL, "Cannot get last element of 0-length name");
	// guaranteed to be an Ident after flattening
	auto p = at(size() - 1);
	return p;
    }
    // the exact opposite of the last function
    // useful for grabbing the namespace from a fully
    // declared member name
    CompoundIdentifier* all_but_last()
    {
	auto ret = new CompoundIdentifier;
	for(unsigned int i = 0; i < size() - 1; i++) {
	    auto item = this->at(i);
	    ret->push_back(item);
	}
	return ret;
    }

    bool equals(CompoundIdentifier* compare)
    {
	if(compare->size() != this->size())
	    return false;
	for(unsigned int i = 0; i < compare->size(); i++) {
	    auto mine = (*this)[i];
	    auto cmp = (*compare)[i];
	    if(mine->name != cmp->name)
		return false;
	}
	return true;
    }
};

class Module;
class Seed;
class TLContainer;

class TLCRef : public Identifier
{
public:
    TLCRef() = default;
    TLCRef(TLContainer* cont)
    {
	this->refersTo = cont;
    }

    CompoundIdentifier* name = nullptr;
    TLContainer* refersTo = nullptr;
    Block<Type> type_params;
    std::vector<Statement*> flatten()
    {
	std::vector<Statement*> ret = { this };
	for(auto m : name->flatten())
	    ret.push_back(m);
	for(auto tp : type_params)
	    for(auto f : tp->flatten())
		ret.push_back(f);
	return ret;
    }
    std::string str()
    {
	std::string ret = name->str();
	if(type_params.size()) {
	    ret += "<";
	    bool first = true;
	    for(auto t : type_params) {
		if(!first)
		    ret += ",";
		ret += t->str();
		first = false;
	    }
	    ret += ">";
	}
	return ret;
    }

    TLCRef* copy(SourceBlock* ctx)
    {
	auto ret = new TLCRef;
	ret->name = name->copy(ctx);
	ret->refersTo = refersTo;
	for(auto tp : type_params) {
	    ret->type_params.push_back(tp->copy(ctx));
	}
	return ret;
    }
};

class SeedRef : public TLCRef
{
public:
    Seed* refersTo;
};
class ModuleRef : public TLCRef
{
public:
    Module* refersTo;
    TLCRef* copy(SourceBlock* ctx)
    {
	except(E_INTERNAL, "Cannot copy ModuleRef");
	return nullptr;
    }

    ModuleRef(Module* to)
    {
	refersTo = to;
    }
    ModuleRef() = default;
};
/**
 *
 * Keep track of a member of a module,
 * with info about the generic params that may be passed
 *
 */
class TLCMemberRef : public Identifier
{
public:
    //
    // if the module is null, it means that the member is part of the global namespace
    //
    TLCRef* parent;
    Ident* member;

    TLCMemberRef(TLCRef* parent, Ident* member)
    {
	this->parent = parent;
	this->member = member;
    }
    TLCMemberRef() = default;
    TLCMemberRef* copy(SourceBlock* ctx)
    {
	auto ret = new TLCMemberRef(parent->copy(ctx), member->copy(ctx));
	return ret;
    }
    llvm::AllocaInst* getPtr(TVars vars)
    {
	auto loaded = vars[str()];

	if(loaded == nullptr)
	    except(E_UNDECLARED_VAR, "Failed to read variable '" + str() + "'");
	return loaded->value;
    }

    llvm::Value* getLLValue(TVars vars, llvm::Type* expected = nullptr)
    {
	auto loaded = getPtr(vars);
	return cast(builder()->CreateLoad(loaded->getType()->getPointerElementType(), loaded), expected);
    }
    std::vector<Statement*> flatten()
    {
	std::vector<Statement*> ret = { this, member };
	if(parent)
	    for(auto m : parent->flatten())
		ret.push_back(m);
	return ret;
    }
    std::string str()
    {
	std::string ret;
	if(parent)
	    ret += parent->str() + ".";
	return ret + member->str();
    }
};