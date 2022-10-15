#pragma once
#include "../token/TokenDef.h"
#include "./ast.hh"
#include <map>
#include <vector>
#define bld (*builder())

static std::map<PrimitiveType, std::string> primitive_group_mappings{
    PRIMITIVES_ENUM_GROUPS};

class MethodCall : public Expression
{
public:
  MethodSignature *target = nullptr;
  CompoundIdentifier *name;
  Block<Expression> params;
  Block<Type> generic_params;
  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> ret{this, name};
    for (auto p : params)
      for (auto f : p->flatten())
        ret.push_back(f);
    return ret;
  }
  Type *getType()
  {
    if(builtin()){
      auto name = this->name->str();
      if(name == "cast")return generic_params[0];
      if(name == "len")return Primitive::get(PR_int64);
      Logger::error("Failed to get return type for builtin " + name);
      return nullptr;
    }
    if (target == nullptr || target->returnType == nullptr)
    {
      Logger::error("Cannot get the return type of an unresolved call");
      return nullptr;
    }
    return target->returnType;
  }
  bool builtin(){
    auto name = this->name->str();
    for(auto d:defs){
      if(d->builtin == name)return true;
      
    }
    return false;
  }
  llvm::Value *getLLValue(TVars vars, llvm::Type *expected = nullptr)
  {
    if(builtin()){
      auto name = this->name->str();
      if(name == "cast"){
        if(params.size() != 1)error("cast<T>() takes one parameter");
        auto type = generic_params[0]->getLLType();
        auto val = params[0]->getLLValue(vars, type);
        return val;
      }
      error("Failed to generate builtin " + name, true);
    }
    if (target == nullptr)
      error("Call to " + name->str() + " is unresolved");
    auto mod = bld.GetInsertBlock()->getParent()->getParent();
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
      llparams.insert(llparams.begin() + idx, bld.getInt32(argCount));
    }

    return bld.CreateCall(tgtFn, llparams);
  }
  // TODO: This method cant really throw exceptions because of how type resolution works
  //  so return a string representing the error reason, these strings can then be accumulated and returned
  //  if it is decided that a type is unresolvable
  void qualify(
      std::map<std::string, MethodSignature *> sigs,
      LocalTypeTable type_info)
  {
    Logger::debug("Qualifying call to " + name->str());
    if (ctx == nullptr)
      error("Cannot Resolve a contextless call");
    if(builtin())return;
    std::vector<Param *> testparams;
    int i = 0;
    for (auto p : params)
    {
      auto type = p->getType();
      if (type == nullptr)
      {
        Logger::error("Failed to get type for param " + name->str() + "[" + std::to_string(i) + "]");
        return;
      }
      testparams.push_back(new Param(type, nullptr));
      i++;
    }
    
    
    // Construct a fake signature to match against
    auto callsig = new MethodSignature;
    callsig->name = name->last();
    callsig->params = testparams;
    callsig->space = name->all_but_last();

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
      // All scores are -1
      if (idx == -1)
      {
        Logger::error("Failed to generate function call to " + callname.str());
        return;
      }
      int ind = 0;
      for (auto pair : sigs)
      {
        if (ind == idx)
        {
          target = pair.second;
          return;
        }
        ind++;
      }
      Logger::error("Somehow failed to find index");
      return;
    }
    target = fn;
  }

private:
  static int getCompat(Type* t1, Type* t2, bool second=false){
    if(t1 == t2)return 0;

    if(t1->primitive() && t2->primitive()){
      auto p1 = t1->primitive();
      auto p2 = t2->primitive();
      auto g1 = primitive_group_mappings[p1->type];
      auto g2 = primitive_group_mappings[p2->type];
      if (g1 == g2)return 1;
      else return -1;
    }
    if(t1->ptr() && t2->ptr()){
      auto p1 = t1->ptr();
      auto p2 = t2->ptr();

      auto r1 = p1->to;
      auto r2 = p2->to;
      return getCompat(r1, r2);
    }
   if(auto cust=t1->custom()){
      auto ref = cust->refersTo;
      if(auto gen = ref->generic()){
        return getCompat(gen->resolveTo, t2);
      }
    }
    if(second)return -1;
    else return getCompat(t2, t1, true);
  }
  static int getCompatabilityScore(QualifiedMethodSigStr base,
                                   QualifiedMethodSigStr target)
  {
    if (base.name->str() != target.name->str())
    {
      return -1;
    }
    // compare namespaces
    if (base.space->str() != target.space->str())
    {
      return -1;
    }

    if (!base.isVariadic())
    {
      if (base.params.size() != target.params.size())
      {
        return -1;
      }
    }


    // We got a generic function
    // Try to decipher each generic type
    if(base.generics.size()){
      if(base.generics.size()>1)error("Functions may only have one generic parameter for the time being");
      if(target.generics.size() > base.generics.size())return -1;

      if(target.generics.size()){
        base.generics[0]->resolveTo = target.generics[0];
      }
      else{
        // find all the params that share the generic type
        // put them into a list, and use the getType method
        std::vector<Type*> sharedTypeParams;
        int i = 0;
        for(auto p:base.params){
          auto dr = p->type->drill();
          if(dr->generic()){
            sharedTypeParams.push_back(target.params[i]->type);
          }
          i++;
        }
        auto genericType = getCommonType(sharedTypeParams);
        Logger::debug("Implicit generic type: " + genericType->str());
        base.generics[0]->resolveTo = genericType;

      }
    }

    // Start with a base score, each infraction has a cost based on how
    // different it is
    int score = 0;
    for (int i = 0; i < target.params.size(); i++)
    {
      auto baram = base.getParam(i)->type;
      auto taram = target.getParam(i)->type;
      auto score = getCompat(baram, taram);
      if(score==-1)return -1;
      else score+=10*score;
    }

    // TODO: Implement Type Reference Inheritance Tree Crawling (Each step up
    // the tree is +10)
    return score;
  }
};

class Return : public Statement
{
private:
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
  bool returns()
  {
    return true;
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
    if (!(instanceof <TPtr>(elementType) || instanceof <ListType>(elementType)))
      error("List has member type which is a non-pointer", true);
    if (instanceof <TPtr>(elementType))
      return ((TPtr *)elementType)->to;
    else
      return ((ListType *)elementType)->elements;
  }
  llvm::Value *getPtr(TVars vars)
  {
    return this->tgt->getPtr(vars);
  }
  llvm::Value *getIdxPtr(TVars vars)
  {
    auto ptr = getPtr(vars);
    auto type = ptr->getType()->getPointerElementType();
    auto idx = getIdx(vars);

    llvm::Value *gep;
    // Load the array directly
    if (type->isArrayTy())
    {
      auto ptrEquiv = type->getArrayElementType()->getPointerTo();
      auto casted = bld.CreateBitCast(ptr, ptrEquiv);
      gep = bld.CreateGEP(ptrEquiv->getPointerElementType(), casted, idx);
    }
    // The array is proxied by an alloca
    else
    {
      auto list = bld.CreateLoad(type, ptr);
      gep = bld.CreateGEP(type->getPointerElementType(), list, idx);
    }
    return gep;
  }
  llvm::Value *getList(TVars vars)
  {
    auto list = this->tgt->getLLValue(vars);
    return list;
  }
  llvm::Value *getIdx(TVars vars)
  {
    return this->item->getLLValue(vars);
  }
  llvm::Type *getElementType(llvm::Value *list)
  {
    auto actualList = list->getType()->getPointerElementType();
    if (actualList->isArrayTy())
      return actualList->getArrayElementType();
    else
      return actualList->getPointerElementType();
  }
  llvm::Value *getLLValue(TVars vars, llvm::Type *target = nullptr)
  {
    auto list = getIdxPtr(vars);
    auto typ = list->getType()->getPointerElementType();
    return cast(bld.CreateLoad(typ, list), target);
  }
  void setAs(llvm::Value *value, TVars vars)
  {
    auto insertPtr = getIdxPtr(vars);
    auto typ = insertPtr->getType()->getPointerElementType();
    bld.CreateStore(cast(value, typ), insertPtr);
    // bld.CreateInsertElement(getPtr(vars), cast(value, typ), idx);
  }
};

enum UnaryOp
{
  UNARY_ENUM_MEMBERS
};
static std::map<TokenType, UnaryOp> prefix_op_mappings{PREFIX_ENUM_MAPPINGS};
static std::map<TokenType, UnaryOp> postfix_op_mappings{POSTFIX_ENUM_MAPPINGS};

enum BinaryOp
{
  INFIX_ENUM_MEMBERS
};

static std::map<TokenType, BinaryOp> binary_op_mappings{INFIX_ENUM_MAPPINGS};

class UnaryOperation : public Expression
{
public:
  Expression *operand;
  UnaryOp op;
  UnaryOperation(Expression *operand, UnaryOp op)
  {
    this->operand = operand;
    this->op = op;
  }
  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> flat = {this};
    for (auto i : operand->flatten())
      flat.push_back(i);
    return flat;
  }
  Type *getType()
  {
    auto i64 = Primitive::get(PR_int64);
    auto boo = Primitive::get(PR_boolean);
    auto same = operand->getType();
    auto sameptr = new TPtr(same);
    Type *pointed = nullptr;
    if (auto pt = same->ptr())
      pointed = pt->to;
    else if (auto pt = same->list())
      pointed = pt->elements;
    switch (op)
    {
    case PRE_amperand:
      return sameptr;
    case PRE_bang:
      return boo;
    case PRE_increment:
      return same;
    case PRE_star:
      return pointed;
    // TODO: this may cause issues in the case of an unsigned integer
    case PRE_minus:
      return same;
    case PRE_bitwise_not:
      return same;
    case PRE_decrement:
      return same;
    case POST_increment:
      return same;
    case POST_decrement:
      return same;
    }
    error("Failed to get type for unary op: " + std::to_string(op));
    return nullptr;
  }
  llvm::Value *getLLValue(TVars types, llvm::Type *expected)
  {
    auto boo = Primitive::get(PR_boolean)->getLLType();
    switch (op)
    {
    case PRE_amperand:
    {
      auto ptr = operand->getPtr(types);
      return cast(ptr, expected);
    }
    case PRE_bang:
      return cast(bld.CreateNot(operand->getLLValue(types, boo)), expected);
    case PRE_minus:
      return cast(bld.CreateNeg(operand->getLLValue(types)), expected);
    case PRE_bitwise_not:
      return cast(bld.CreateNot(operand->getLLValue(types)), expected);
    case PRE_star:
    {
      auto typ = operand->getType();
      if (typ->list())
      {
        auto ptr = operand->getPtr(types);
        auto el = ptr->getType()->getPointerElementType();
        auto gep = bld.CreateConstGEP2_32(el, ptr, 0, 0);
        auto load = bld.CreateLoad(gep->getType()->getPointerElementType(), gep);
        return cast(load, expected);
      }
      else if (typ->ptr())
      {
        auto val = operand->getLLValue(types);
        auto loaded = bld.CreateLoad(val->getType()->getPointerElementType(), val);
        return cast(loaded, expected);
      }
      else
        error("Cannot dereference type: " + typ->str());
    }
    case PRE_increment:
    {
      if (! instanceof <Ident>(operand))
        error("Cannot Increment non-identifiers");
      auto var = (Ident *)operand;
      auto val = var->getLLValue(types);
      auto typ = val->getType();
      auto ptr = var->getPtr(types);
      auto inc = bld.CreateAdd(val, cast(bld.getInt32(1), typ));
      bld.CreateStore(inc, ptr);
      return cast(inc, expected);
    }
    case PRE_decrement:
    {
      if (! instanceof <Ident>(operand))
        error("Cannot Increment non-identifiers");
      auto var = (Ident *)operand;
      auto val = var->getLLValue(types);
      auto typ = val->getType();
      auto ptr = var->getPtr(types);
      auto inc = bld.CreateSub(val, cast(bld.getInt32(1), typ));
      bld.CreateStore(inc, ptr);
      return cast(inc, expected);
    }
    case POST_increment:
    {
      if (! instanceof <Ident>(operand))
        error("Cannot Increment non-identifiers");
      auto var = (Ident *)operand;
      auto val = var->getLLValue(types);
      auto typ = val->getType();
      auto ptr = var->getPtr(types);
      auto inc = bld.CreateAdd(val, cast(bld.getInt32(1), typ));
      bld.CreateStore(inc, ptr);
      return cast(val, expected);
    }
    case POST_decrement:
    {
      if (! instanceof <Ident>(operand))
        error("Cannot Increment non-identifiers");
      auto var = (Ident *)operand;
      auto val = var->getLLValue(types);
      auto typ = val->getType();
      auto ptr = var->getPtr(types);
      auto inc = bld.CreateSub(val, cast(bld.getInt32(1), typ));
      bld.CreateStore(inc, ptr);
      return cast(val, expected);
    }
    }
    error("Failed to generate llvalue for unary operation: " + std::to_string(op));
  }
};

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
    auto rt = right->getType();
    auto lt = left->getType();

    auto common_t = getCommonType(lt, rt);
    auto common = common_t->getLLType();

    if (op == BIN_assignment)
    {
      if (instanceof <Ident>(left))
      {
        auto id = (Ident *)left;
        auto ptr = id->getPtr(types);
        auto typ = ptr->getType()->getPointerElementType();
        auto r = right->getLLValue(types, typ);
        if (r->getType()->isPointerTy())
        {
          auto elType = r->getType()->getPointerElementType();
          if (elType->isArrayTy())
          {
            // Allocating array to variable, memcpy
            auto size = elType->getArrayNumElements() * (elType->getArrayElementType()->getPrimitiveSizeInBits() / 8);

            auto leftAlign = ptr->getAlign();
            // auto rightAlign = ((llvm::Constant*)casted)->getAlign();
            bld.CreateMemCpy(ptr, leftAlign, r, leftAlign, size);
          }
          else
            bld.CreateStore(cast(r, typ), ptr);
        }
        else{
          bld.CreateStore(cast(r, typ), ptr);
        }
        return cast(r, expected);
      }
      else if (instanceof <Subscript>(left))
      {
        auto r = right->getLLValue(types);
        auto sub = (Subscript *)left;
        sub->setAs(r, types);
        return cast(r, expected);
      }
      else
      {
        error("Failed to generate Assignment");
      }
    }
    auto l = left->getLLValue(types, common);
    auto r = right->getLLValue(types, common);
    auto result = getOp(l, r);
    if (result == nullptr)
      error("Failed to generate IR for binary expression");
    auto casted = cast(result, expected);
    return casted;
  }

private:
  llvm::Value *getOp(llvm::Value *l, llvm::Value *r)
  {
#define b return bld
    switch (op)
    {
    case BIN_assignment:
      error("Assignment Operators are not supported by this function");
    case BIN_dot:
      error("Dot Operators are not supported by this function");
    case BIN_plus:
      b.CreateAdd(l, r);
    case BIN_minus:
      b.CreateSub(l, r);
    case BIN_star:
      b.CreateMul(l, r);
    case BIN_slash:
      b.CreateSDiv(l, r);
    case BIN_percent:
      b.CreateSRem(l, r);

    case BIN_lesser:
      b.CreateICmpSLT(l, r);
    case BIN_greater:
      b.CreateICmpSGT(l, r);
    case BIN_lesser_eq:
      b.CreateICmpSLE(l, r);
    case BIN_greater_eq:
      b.CreateICmpSGE(l, r);
    case BIN_not_equals:
      b.CreateICmpNE(l, r);
    case BIN_equals:
      b.CreateICmpEQ(l, r);

    case BIN_bitiwse_or:
      b.CreateOr(l, r);
    case BIN_bitwise_and:
      b.CreateAnd(l, r);
    case BIN_bitwise_shl:
      b.CreateShl(l, r);
    case BIN_bitwise_shr:
      b.CreateAShr(l, r);
    case BIN_bitwise_xor:
      b.CreateXor(l, r);

    case BIN_bool_and:
      b.CreateLogicalAnd(l, r);
    case BIN_bool_or:
      b.CreateLogicalOr(l, r);
    }
  }
};

class InitializeVar : public Statement
{
public:
  Type *type;
  Identifier *varname;
  Expression *initializer = nullptr;
  InitializeVar(Type *t, Identifier *name, Expression *initializer = nullptr)
  {
    type = t;
    this->initializer = initializer;
    varname = name;
  }
  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> ret{this, varname};
    if (initializer)
      for (auto i : initializer->flatten())
        ret.push_back(i);
    return ret;
  }
  std::string str()
  {
    return "let " + varname->str() + " : " + type->str();
  }
};
