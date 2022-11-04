#pragma once
#include "../../lib/error.h"
#include "llvm/IR/Type.h"
#include <string>
#include <vector>
class AstNode
{
public:
    virtual ~AstNode() {

    };
};

class SourceBlock;

struct Statement : public AstNode {
public:
    // Statements can be deactivated if needed
    // This is useful when working with flattened asts, where it is impossible to remove nodes
    // The next best thing is deactivation
    bool active = true;
    SourceBlock* ctx = nullptr;
    virtual std::vector<Statement*> flatten()
    {
	except(E_INTERNAL, "Cannot Flatten a raw Statement");
	return {};
    }

    virtual Statement* copy(SourceBlock* ctx)
    {
	except(E_INTERNAL, "Cannot Copy base Statement Type");
	return nullptr;
    }

    Statement() = default;
};

class Primitive;
class ListType;
class TPtr;
class CustomType;
class Generic;
class ModuleInstanceType;
class TLCType;
struct Type : public Statement {
public:
    virtual std::string str()
    {
	except(E_INTERNAL, "Cannot stringify base type");
	return "";
    }
    virtual llvm::Type* getLLType()
    {
	except(E_INTERNAL, "Cannot get LL Type for base Type Class");
	return nullptr;
    }
    std::vector<Statement*> flatten()
    {
	except(E_INTERNAL, "Cannot Flatten Base Type");
	return {};
    }

    // Return the deepest possble type from a potentially complex
    // type tree, useful for generic detection amongst other things
    virtual Type* drill()
    {
	except(E_INTERNAL, "Cannot drill base type class");
	return nullptr;
    }

    //
    // Produce a deep copy of a type tree
    //
    Type* copy(SourceBlock* ctx)
    {
	except(E_INTERNAL, "Cannot copy base type");
	return nullptr;
    }
    virtual std::pair<Type*, Type*> find_mismatch(Type* _)
    {
	except(E_INTERNAL, "Cannot differ types against base type object");
	return {};
    }
    virtual Primitive* primitive()
    {
	return nullptr;
    }
    virtual CustomType* custom()
    {
	return nullptr;
    }
    virtual TPtr* ptr()
    {
	return nullptr;
    }
    virtual ListType* list()
    {
	return nullptr;
    }
    virtual Generic* generic()
    {
	return nullptr;
    }
    virtual ModuleInstanceType* inst()
    {
	return nullptr;
    }

    virtual TLCType* mod()
    {
	return nullptr;
    }
};
class Constant;
struct Expression : public Statement {
public:
    virtual Type* getType()
    {
	except(E_INTERNAL, "GetType Fn is not implemented for Expression");
	return nullptr;
    };

    virtual llvm::Value* getLLValue(TVars _vars, llvm::Type* _expected = nullptr)
    {
	except(E_INTERNAL, "Cannot get llvm value for base expression type");
	return nullptr;
    }
    virtual llvm::Value* getPtr(TVars _vars)
    {
	except(E_INTERNAL, "Cannot get ptr to base expression type");
	return nullptr;
    }
    virtual Constant* constant()
    {
	return nullptr;
    }
    Expression* copy(SourceBlock* ctx)
    {
	except(E_INTERNAL, "Cannot copy base expression type");
	return nullptr;
    }
};

template <typename T> class Block : public AstNode, public std::vector<T*>
{
public:
    std::vector<T*> take()
    {
	destroy = false;
	return *this;
    }
    Block(bool memoryAware)
    {
	destroy = memoryAware;
    }
    Block(std::vector<T*> init)
    {
	for(auto item : init)
	    this->push_back(item);
    }
    Block() = default;
    ~Block()
    {
	if(destroy) {
	    for(auto i : *this) {
		delete i;
	    }
	}
    }

private:
    bool destroy = false;
};

class TopLevelMetadata;
struct CompilationUnit;
struct TopLevelExpression : public AstNode {
public:
    CompilationUnit* unit;
    virtual bool isModule()
    {
	return false;
    }
    virtual bool isImport()
    {
	return false;
    }
    bool isImported = false;
};

class TLContainer;
struct ModuleMember : public AstNode {
public:
    TLContainer* memberOf = nullptr;
    Block<TopLevelMetadata> metadata;
    bool public_access = false;
    bool instance_access = false;
};
// A Souceblock is a wrapper around a statement block
// but contains important information such as guarantees about execution
// and a local scope type table
// variables cannot be redefined within the same block and are hence guaranteed to keep the same type
class SourceBlock : public Block<Statement>, public Statement
{
public:
    SourceBlock(std::vector<Statement*> items)
        : Block(items)
    {
	this->local_types = new LocalTypeTable;
    }
    SourceBlock(Statement* item)
    {
	this->push_back(item);
	this->local_types = new LocalTypeTable;
    }
    ~SourceBlock()
    {
	delete local_types;
    }
    SourceBlock* copy(SourceBlock* ctx)
    {
	auto sb = new SourceBlock;
	auto tt = new LocalTypeTable;
	*tt = *this->local_types;
	sb->ctx = ctx;
	sb->local_types = tt;
	for(auto c : *this) {
	    sb->push_back(c->copy(sb));
	}
	return sb;
    }

    SourceBlock() = default;
    LocalTypeTable* local_types = nullptr;

    Type* getType(std::string var)
    {
	if(!local_types)
	    except(E_INTERNAL, "No Type Table?");
	auto ty = (*local_types)[var];
	if(!ty && ctx) {
	    return ctx->getType(var);
	}
	return ty;
    }

    std::vector<Statement*> flatten()
    {
	std::vector<Statement*> ret = { this };
	for(auto i : *this) {
	    for(auto m : i->flatten()) {
		ret.push_back(m);
	    }
	}
	return ret;
    }

    void gobble(SourceBlock* donor)
    {
	if(!donor)
	    except(E_INTERNAL, "Cannot merge with a null donor");
	auto old = donor;
	for(auto item : donor->take()) {
	    // update the ctx
	    for(auto i : item->flatten()) {
		if(i->ctx == old) {
		    i->ctx = this;
		}
	    }
	    item->ctx = this;
	    this->push_back(item);
	}
	for(auto pair : *donor->local_types) {
	    if(!local_types)
		except(E_INTERNAL, "My locals are null?");
	    auto my = *local_types;
	    if(my[pair.first] == nullptr) {
		(*local_types)[pair.first] = pair.second;
	    }
	}
    }
};