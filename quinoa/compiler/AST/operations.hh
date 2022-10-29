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
  bool inst = false;
  MethodSignature *target = nullptr;

  ModuleMemberRef *name;
  Block<Expression> params;
  Block<Type> generic_params;

  MethodCall *copy(SourceBlock *ctx)
  {
    auto c = new MethodCall;
    c->ctx = ctx;
    c->name = name->copy(ctx);
    c->target = target;
    for (auto p : params)
    {
      c->params.push_back(p->copy(ctx));
    }
    for (auto g : generic_params)
    {
      c->generic_params.push_back(g->copy(ctx));
    }
    return c;
  }

  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> ret{this};
    for (auto p : params)
      for (auto f : p->flatten())
        ret.push_back(f);
    for (auto m : name->flatten())
      ret.push_back(m);
    for (auto t : generic_params)
    {
      for (auto f : t->flatten())
        ret.push_back(f);
    }
    return ret;
  }
  Type *getType()
  {
    if (builtin())
    {
      auto name = this->name->str();
      if (name == "cast")
        return generic_params[0];
      if (name == "len")
        return Primitive::get(PR_int64);
      Logger::error("Failed to get return type for builtin " + name);
      return nullptr;
    }
    if (target == nullptr || target->returnType == nullptr)
    {
      Logger::error("Cannot get the return type of an unresolved call to " + name->str());
      return nullptr;
    }
    return target->returnType;
  }
  bool builtin()
  {
    auto name = this->name->str();
    for (auto d : defs)
    {

      if (d->builtin == name)
        return true;
    }
    return false;
  }
  llvm::Value *getLLValue(TVars vars, llvm::Type *expected = nullptr)
  {
    if (builtin())
    {
      auto name = this->name->str();
      if (name == "cast")
      {
        if (params.size() != 1)
          error("cast<T>() takes one parameter");
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
    {
      error("Failed to locate function " + name);
    }
    std::vector<llvm::Value *> llparams;
    int i = 0;
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
  Block<Generic> make_generic_refs()
  {
    Block<Generic> g;

    for (auto gp : generic_params)
    {
      auto gen = new Generic(Ident::get("Placeholder_Generic"));
      gen->refersTo = gp;
      g.push_back(gen);
    }

    return g.take();
  }
};

class Return : public Statement
{
private:
public:
  Expression *retValue;
  Return(Expression *value) { this->retValue = value; }
  Return() = default;
  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> ret{this};
    if (retValue)
      for (auto i : retValue->flatten())
        ret.push_back(i);
    return ret;
  }
  bool returns()
  {
    return true;
  }

  Return *copy(SourceBlock *ctx)
  {
    auto r = new Return;
    if (retValue)
      r->retValue = retValue;
    r->ctx = ctx;
    return r;
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
    auto boo = Primitive::get(PR_boolean);
    auto same = operand->getType();
    auto sameptr = new TPtr(same);
    Type *pointed = nullptr;
    if(same){
    if (auto pt = same->ptr())
      pointed = pt->to;
    else if (auto pt = same->list())
      pointed = pt->elements;}
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

  llvm::Value *getPtr(TVars vars)
  {
    switch (op)
    {

    default:
    {
    case PRE_star:
      return operand->getLLValue(vars, nullptr);
      error("Cannot get pointer to unary operation of type " + std::to_string(op));
      return nullptr;
    }
    }
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
  llvm::Value *getPtr(TVars types)
  {
    if (op == BIN_dot)
    {
      auto struct_ref = left->getPtr(types);
      auto name = dynamic_cast<Ident *>(right);
      if (!name)
        error("Module references may only be indexed by identifiers");
      if (!struct_ref->getType()->getPointerElementType()->isStructTy())
        error("You can only use dot notation on module references");
      auto mod = left->getType()->drill()->inst()->of->drill()->mod()->ref->refersTo;

      // Convert the name into a struct index, and GEP + Load it
      auto idx = getModuleMemberIdx(mod, name->str());
      if (idx == (size_t)-1)
        error("Failed to get prop " + name->str());

      auto struct_type = struct_ref->getType()->getPointerElementType();
      auto elementPtr = bld.CreateConstInBoundsGEP2_32(struct_type, struct_ref, 0, idx);
      return elementPtr;
    }
    error("Cannot get Pointer to Binop of type " + std::to_string(op));
    return nullptr;
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
    switch (op)
    {
    case BIN_plus:
    case BIN_minus:
    case BIN_star:
    case BIN_slash:
    case BIN_percent:
    case BIN_bitiwse_or:
    case BIN_bitwise_and:
    case BIN_bitwise_shl:
    case BIN_bitwise_shr:
    case BIN_bitwise_xor:
    case BIN_bool_and:
    case BIN_bool_or:
    case BIN_greater:
    case BIN_greater_eq:
    case BIN_equals:
    case BIN_not_equals:
    case BIN_lesser:
    case BIN_lesser_eq:
      return getCommonType(left->getType(), right->getType());
    case BIN_assignment:
      return right->getType();
    case BIN_dot:
    {
      if(auto func = dynamic_cast<MethodCall*>(right)){
        return func->getType();
      }
      auto parent_struct_type = left->getType();
      if (parent_struct_type == nullptr)
        return nullptr;
      auto inst = parent_struct_type->drill()->inst();
      if (!inst)
        error("You can only use dot-notation on module instances");

      auto parent = inst->of->drill()->mod();
      if (!parent)
        error("Unresolved Module?");
      auto mod = parent->ref->refersTo;
      // Property Access
      if (auto member = dynamic_cast<Ident *>(right))
      {
        auto prop = getProperty(mod, member->str());
        if (!prop)
          error("Failed to get property " + mod->name->str() + "." + member->str());
        if (!prop->instance_access)
          error("Cannot access non-instance property " + prop->str() + " from module instance");
        return prop->type;
      }
      else
        error("You may only use identifiers to index module instances");
    }
    }
    error("Failed to get type for op: " + std::to_string(op));
    return this->right->getType();
  }
  BinaryOperation *copy(SourceBlock *ctx)
  {
    auto bop = new BinaryOperation(left->copy(ctx), right->copy(ctx), op);
    bop->ctx = ctx;
    return bop;
  }
  llvm::Value *getLLValue(TVars types, llvm::Type *expected)
  {
    if (op == BIN_dot)
    {
      if(auto func = dynamic_cast<MethodCall*>(right)){
        return func->getLLValue(types, expected);
      }
      auto ptr = getPtr(types);
      auto val = bld.CreateLoad(ptr->getType()->getPointerElementType(), ptr);
      return val;
    }

    auto rt = right->getType();
    auto lt = left->getType();

    if(lt==nullptr)error("Failed to get type of left-hand operand");
    if(rt==nullptr)error("Failed to get type of right-hand operand");

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
        else
        {
          bld.CreateStore(cast(r, typ), ptr);
        }
        return cast(r, expected);
      }
      if (instanceof <Subscript>(left))
      {
        auto r = right->getLLValue(types);
        auto sub = (Subscript *)left;
        sub->setAs(r, types);
        return cast(r, expected);
      }

      if (auto struc_expr = dynamic_cast<BinaryOperation *>(left))
      {
        if (struc_expr->op == BIN_dot)
        {
          auto r = right->getLLValue(types);
          auto member = struc_expr->getPtr(types);
          auto val = cast(r, member->getType()->getPointerElementType());
          bld.CreateStore(val, member);
          return cast(r, expected);
        }
      }
      error("Failed to generate Assignment");
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
    if (type)
      for (auto f : type->flatten())
        ret.push_back(f);
    return ret;
  }
  std::string str()
  {
    return "let " + varname->str() + " : " + type->str();
  }
};
