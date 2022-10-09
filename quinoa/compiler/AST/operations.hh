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
  CompoundIdentifier *name;
  std::vector<Expression *> params;

  std::vector<Statement *> flatten()
  {
    std::vector<Statement *> ret{this, name};
    for (auto p : params)
      ret.push_back(p);
    return ret;
  }
  Type *getType(LocalTypeTable _)
  {
    if (target == nullptr || target->returnType == nullptr)
      return nullptr;
    return target->returnType;
  }
  void qualify(
      std::map<std::string, MethodSignature *> sigs,
      LocalTypeTable type_info)
  {

    std::vector<Param *> testparams;
    for (auto p : params){
      auto type = p->getType(type_info);
      if(type==nullptr)error("Unknown param type");
      testparams.push_back(new Param(type, nullptr));
    }
    auto callsig = new MethodSignature;
    callsig->name = name->last();
    callsig->params = testparams;
    callsig->space = name->all_but_last();
    auto sigstr = callsig->sigstr();

    CompoundIdentifier callname(name->parts);
    // replace the calls name with its mangled form
    callname.parts.pop_back();
    callname.parts.push_back((Identifier *)Ident::get(sigstr.str()));
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
      if (idx == -1){
        error("Failed to generate function call to " + callname.str());
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
      return;
    }
    target = fn;
  }

private:
  static int getCompatabilityScore(QualifiedMethodSigStr base,
                                   QualifiedMethodSigStr target)
  {
    Logger::debug("Comparing " + base.str() + " against " + target.str());
    if (base.name->str() != target.name->str())
      return -1;
    // compare namespaces
    if (base.space->str() != target.space->str())
      return -1;

    // compare param lengths TODO: Reimplement this once varargs are implemented
    if(!base.isVariadic()){
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
      if (baram->equals(taram))
        continue;
      if (instanceof <Primitive>(baram) && instanceof <Primitive>(taram))
      {
        // same group, different type is +10, otherwise no match

        auto bprim = (Primitive *)baram;
        auto tprim = (Primitive *)taram;
        auto bg = primitive_group_mappings[bprim->type];
        auto tg = primitive_group_mappings[tprim->type];
        if (bg == tg){

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
};

class TypeCast : public Expression
{
public:
  Expression *target;
  Type *to;

  TypeCast(Expression *tgt, Type *type)
  {
    this->to = type;
    this->target = tgt;
  }
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

  Type *getType(LocalTypeTable tt)
  {
    Logger::debug("im in");
    Logger::debug("Getting type of sub of " + tgt->str());
    auto elementType = tgt->getType(tt);
    if(elementType==nullptr)error("No Element Type");
    Logger::debug(tgt->str() + " : " + elementType->str());
    if(!instanceof<TPtr>(elementType))error("List has member type which is a non-pointer");
    return ((TPtr*)elementType)->to;
  }
};