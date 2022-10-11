#pragma once
#include "../token/TokenDef.h"
#include "./ast.hh"
#include <map>
#include <vector>

static std::map<PrimitiveType, std::string> primitive_group_mappings{
    PRIMITIVES_ENUM_GROUPS};

class MethodCall : public Expression
{
public:
  MethodSignature *target = nullptr;
  bool nomangle = false;
  CompoundIdentifier *name;
  std::vector<Expression *> params;

  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> ret{this, name};
    for (auto p : params)
      ret.push_back(p);
    return ret;
  }
  Type *getType()
  {
    if (target == nullptr || target->returnType == nullptr)
    {
      // error("Cannot get the return type of an unresolved call", true);
      return nullptr;

    }
    return target->returnType;
  }

  llvm::Value *getLLValue(TVars vars, llvm::Type *expected = nullptr)
  {
    if (target == nullptr)
      error("Call to " + name->str() + " is unresolved");
    auto mod = builder()->GetInsertBlock()->getParent()->getParent();
    auto name = target->nomangle ? target->name->str() : target->sourcename();
    auto tgtFn = mod->getFunction(name);
    if (tgtFn == nullptr)
      error("Failed to locate function " + name);
    std::vector<llvm::Value *> llparams;
    int i = 0;
    bool generatedVarargs = false;
    for (auto p : this->params)
    {
      auto type = target->getParam(i)->type;
      auto ll_type = type->getLLType();
      llparams.push_back(p->getLLValue(vars, ll_type));
      i++;
    }
    if (target->isVariadic() && !target->nomangle)
    {
      int idx = target->params.size() - 1;
      int argCount = params.size() - idx;
      // Insert the arg_count parameter before the varargs
      llparams.insert(llparams.begin() + idx, builder()->getInt32(argCount));
    }

    return builder()->CreateCall(tgtFn, llparams);
  }
  //TODO: This method cant really throw exceptions because of how type resolution works
  // so return a string representing the error reason, these strings can then be accumulated and returned
  // if it is decided that a type is unresolvable
  void qualify(
      std::map<std::string, MethodSignature *> sigs,
      LocalTypeTable type_info)
  {
    Logger::debug("resolving call to " + name->str());
    if(ctx==nullptr)error("Cannot Resolve a contextless call");
    if (nomangle)
    {
      for (auto pair : sigs)
      {
        auto signame = pair.first;
        auto sig = pair.second;
        if (signame == name->str() && sig->nomangle)
        {
          target = sig;
          return;
        }
      }
      error("Failed to find appropriate function call for internal method");
    }
    std::vector<Param *> testparams;
    int i = 0;
    for (auto p : params)
    {
      auto type = p->getType();
      if (type == nullptr){
        Logger::debug("Failed to get type for param " + std::to_string(i));
        return;
      }
      testparams.push_back(new Param(type, nullptr));
      i++;
    }
    auto callsig = new MethodSignature;
    callsig->name = name->last();
    callsig->params = testparams;
    callsig->space = name->all_but_last();
    callsig->nomangle = nomangle;
    auto sigstr = callsig->sigstr();

    CompoundIdentifier callname(name->parts);
    // replace the calls name with its mangled form
    callname.parts.pop_back();
    callname.parts.push_back((Identifier *)Ident::get(sigstr.str(), ctx));
    // attempt to find a function with the exact sig

    auto fn = sigs[callname.str()];
    if (fn == nullptr)
    {
      sigs.erase(callname.str());
      // Run Compatibility Checks on each sigstr pair to find
      // the most compatible function to match to
      std::vector<int> compatabilityScores = {};
      for (auto sigpair : sigs)
      {
        auto sig = sigpair.second;
        auto name = sigpair.first;
        if (sig->nomangle)
        {
          compatabilityScores.push_back(-1);
          continue;
        }
        int compat = getCompatabilityScore(sig->sigstr(), callsig->sigstr());
        Logger::debug("Compat with " + name + " : " + std::to_string(compat));
        compatabilityScores.push_back(compat);
      }
      int max = -1;
      int prev = -1;
      int idx = -1;
      for (int i = 0; i < compatabilityScores.size(); i++)
      {
        auto s = compatabilityScores[i];
        if (s == -1)
          continue;
        if (s <= max || max == -1)
        {
          prev = max;
          max = s;
          idx = i;
        }
      }
      if (idx == -1)
      {
        // error("Failed to generate function call to " + callname.str());
        return;
      }
      int ind = 0;
      for (auto pair : sigs)
      {
        if (ind == idx)
        {
          Logger::debug("Casted Match");
          target = pair.second;
          return;
        }
        ind++;
      }
      return;
    }
    Logger::debug("Direct Match");
    target = fn;
  }

private:
  static int getCompatabilityScore(QualifiedMethodSigStr base,
                                   QualifiedMethodSigStr target)
  {
    if (base.name->str() != target.name->str()){
      return -1;

    }
    // compare namespaces
    if (base.space->str() != target.space->str()){
      Logger::debug("Namespaces don't match");
      Logger::debug(base.space->str());
      Logger::debug(target.space->str());
      return -1;

    }
    Logger::debug("compatible name");

    // compare param lengths TODO: Reimplement this once varargs are implemented
    if (!base.isVariadic())
    {
      if (base.params.size() != target.params.size())
        return -1;
    }

    // Start with a base score, each infraction has a cost based on how
    // different it is
    int score = 0;
    for (int i = 0; i < target.params.size(); i++)
    {
      auto baram = base.getParam(i)->type;
      auto taram = target.getParam(i)->type;
      if (baram == taram)
        continue;
      if (instanceof <Primitive>(baram) && instanceof <Primitive>(taram))
      {
        // same group, different type is +10, otherwise no match

        auto bprim = (Primitive *)baram;
        auto tprim = (Primitive *)taram;
        auto bg = primitive_group_mappings[bprim->type];
        auto tg = primitive_group_mappings[tprim->type];
        if (bg == tg)
        {

          score += 10;
        }
        else
          score = -1;
      }
    }

    // TODO: Implement Type Reference Inheritance Tree Crawling (Each step up
    // the tree is +10)
    return score;
  }
};

class Return : public Statement
{
public:
  Expression *retValue;
  Return(Expression *value) { this->retValue = value; }
  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> ret{this};
    for (auto i : retValue->flatten())
      ret.push_back(i);
    return ret;
  }
};

class Subscript : public Expression
{
public:
  Identifier *tgt;
  Expression *item;
  Subscript(Identifier *tgt, Expression *item)
  {
    this->tgt = tgt;
    this->item = item;
  }
  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> ret{tgt};
    for (auto p : item->flatten())
      ret.push_back(p);
    return ret;
  }

  Type *getType()
  {
    auto elementType = tgt->getType();
    if (elementType == nullptr)
      error("No Element Type");
    Logger::debug("Child type: " + elementType->str());
    if (!(instanceof <TPtr>(elementType) || instanceof<ListType>(elementType)))
      error("List has member type which is a non-pointer", true);
    if(instanceof<TPtr>(elementType))return ((TPtr *)elementType)->to;
    else return ((ListType*)elementType)->elements;
  }

  llvm::Value *getPtr(TVars vars, llvm::Type *target = nullptr)
  {
    auto varPtr = tgt->getPtr(vars);
    
    auto idx = item->getLLValue(vars);
    auto loaded = builder()->CreateGEP(varPtr->getType()->getPointerElementType(), varPtr, idx, "subscript-ptr");
    return loaded;
  }
  llvm::Value *getLLValue(TVars vars, llvm::Type *target = nullptr)
  {
    auto loaded = getPtr(vars, target);
    return cast(builder()->CreateLoad(loaded->getType()->getPointerElementType(), loaded), target);
  }
};

class ArrayLength : public Expression
{
public:
  Identifier *of;
  ArrayLength(Identifier *of)
  {
    this->of = of;
  }
  std::vector<Statement *> flatten()
  {
    return {this, of};
  }
  Type *getType()
  {
    return Primitive::get(PR_int64);
  }
  llvm::Value *getLLValue(TVars vars, llvm::Type *target = nullptr)
  {
    auto alloca = of->getPtr(vars);
    auto size = alloca->getArraySize();
    return cast(size, target);
  }
};
enum BinaryOp
{
  INFIX_ENUM_MEMBERS
};

static std::map<TokenType, BinaryOp> binary_op_mappings{INFIX_ENUM_MAPPINGS};
class BinaryOperation : public Expression
{
public:
  Expression *left;
  Expression *right;
  BinaryOp op;

  BinaryOperation(Expression *left, Expression *right, BinaryOp op)
  {
    this->left = left;
    this->right = right;
    this->op = op;
  }
  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> flat = {this};
    auto l = left->flatten();
    for (auto i : l)
      flat.push_back(i);
    auto r = right->flatten();
    for (auto i : r)
      flat.push_back(i);
    return flat;
  }
  Type *getType()
  {
    // TODO: Fix this up
    return this->right->getType();
  }

  llvm::Value *getLLValue(TVars types, llvm::Type *expected)
  {
    auto lt = left->getType();
    auto rt = right->getType();
    auto common = getCommonType(lt->getLLType(), rt->getLLType());
    
    auto l = left->getLLValue(types, common);
    auto r = right->getLLValue(types, common);

    switch (op)
    {
    case BIN_assignment:
    {
      if (instanceof <Ident>(left))
      {
        auto id = (Ident *)left;
        auto ptr = id->getPtr(types);
        auto typ = ptr->getType()->getPointerElementType();
        builder()->CreateStore(right->getLLValue(types, typ), ptr);
      }
      if (instanceof <Subscript>(left)){
        auto sub = (Subscript*)left;
        auto ptr = sub->getPtr(types);
        auto typ = sub->getLLValue(types)->getType()->getPointerElementType();
        builder()->CreateStore(right->getLLValue(types, typ), sub->getPtr(types));
      }
      return cast(r, expected);
    };
    case BIN_plus:{
      return cast(builder()->CreateAdd(l, r), expected);

    }
    case BIN_lesser:
      return cast(builder()->CreateICmpSLT(l, r), expected);
    case BIN_not_equals: return cast(builder()->CreateICmpNE(l, r), expected);
    default:
      error("Failed to generate IR for binary expression");
    }
    error("Failed to generate binary expression " + std::to_string(op));
    return nullptr;
  }

private:


};

class InitializeVar : public Statement
{
public:
  Type *type;
  Identifier *varname;
  InitializeVar(Type *t, Identifier *name)
  {
    type = t;
    varname = name;
  }
  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> ret{this};
    for (auto i : varname->flatten())
      ret.push_back(i);
    return ret;
  }
};
