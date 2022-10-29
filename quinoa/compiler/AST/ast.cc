#include "./ast.hh"

Type *getCommonType(Type *_t1, Type *_t2, bool second_pass)
{
  if (_t1 == nullptr || _t2 == nullptr)
    error("one of the types is null", true);
  auto t1 = _t1->drill();
  auto t2 = _t2->drill();

  if (t1 == t2)
    return t1;

  if (t1->primitive() && t2->primitive())
  {
    auto t1p = t1->primitive();
    auto t2p = t2->primitive();
    return t1p->getMutual(t2p);
  }
  if (t1->list() && t2->list())
  {
    auto t1l = t1->list()->elements;
    auto t2l = t2->list()->elements;
    auto mut = getCommonType(t1l, t2l);
    return new ListType(mut);
  }
  if (t1->ptr() && t2->ptr())
  {
    auto p1 = t1->ptr();
    auto p2 = t2->ptr();
    return new TPtr(getCommonType(p1->to, p2->to));
  }
  if (t1->inst() && t2->inst())
  {
    auto i1 = t1->inst()->of->drill();
    auto i2 = t2->inst()->of->drill();
    if (i1->mod() && i2->mod())
    {
      auto m1 = i1->mod()->ref->refersTo;
      auto m2 = i2->mod()->ref->refersTo;
      if (m1 == m2)
        return t1;
    }
  }
  if (t1->ptr() && t2->list())
  {
    auto t1p = t1->ptr();
    auto t2l = t2->list();
    auto mem = getCommonType(t1p->to, t2l->elements);
    return new TPtr(mem);
  }
  if (second_pass)
    error("Failed To Get Common Type Between " + t1->str() + " and " + t2->str(), true);
  return getCommonType(t2, t1, true);
}
Type *getCommonType(std::vector<Type *> items)
{
  if (items.size() == 1)
    return items[0];
  if (items.size() == 0)
    error("Cannot get type of list with 0 elements");

  // Divide and conquer
  auto splitIdx = items.size() / 2;
  auto beg = items.begin();
  auto end = items.end();
  auto left = std::vector<Type *>(beg, beg + splitIdx);
  auto right = std::vector<Type *>(beg + splitIdx, end);

  auto left_t = getCommonType(left);
  auto right_t = getCommonType(right);

  return getCommonType(left_t, right_t);
}
Type *getCommonType(std::vector<Expression *> items)
{
  std::vector<Type *> types;
  for (auto i : items)
    types.push_back(i->getType());
  return getCommonType(types);
}

llvm::StructType *structify(Module *mod)
{
  if (mod->struct_type)
    return mod->struct_type;

  std::vector<llvm::Type *> struct_elements;

  auto struct_name = "struct." + mod->fullname()->str();

  for (auto prop : mod->getAllProperties())
  {
    if (!prop->instance_access)
      continue;
    auto prop_type = prop->type->getLLType();
    struct_elements.push_back(prop_type);
  }
  auto struct_type = llvm::StructType::create(*llctx(), struct_elements, struct_name);
  mod->struct_type = struct_type;
  return struct_type;
}
Property *getProperty(Module *mod, std::string propname)
{
  for (auto p : mod->getAllProperties())
  {
    if (p->name->member->str() == propname)
      return p;
  }
  return nullptr;
}

int getCompat(ModuleRef *r1, ModuleRef *r2)
{
  if (!r1 || !r2)
    return -1;
  if (!r1->refersTo || !r2->refersTo)
    return -1;

  auto m1 = r1->refersTo;
  auto m2 = r2->refersTo;
  if (m1 == m2)
    return 0;
  // TODO: Crawl The Inheritance Tree, +1 for each level
  return -1;
}
int getCompat(Type *_t1, Type *_t2, bool second = false)
{
  auto t1 = _t1->drill();
  auto t2 = _t2->drill();
  if (t1 == t2)
    return 0;

  if (t1->primitive() && t2->primitive())
  {
    auto p1 = t1->primitive();
    auto p2 = t2->primitive();
    auto g1 = primitive_group_mappings[p1->type];
    auto g2 = primitive_group_mappings[p2->type];
    if (g1 == g2)
      return 1;
    else
      return -1;
  }
  if (t1->ptr() && t2->ptr())
  {
    auto p1 = t1->ptr();
    auto p2 = t2->ptr();

    auto r1 = p1->to;
    auto r2 = p2->to;
    return getCompat(r1, r2);
  }
  if (t1->list() && t2->list())
  {
    auto a1 = t1->list();
    auto a2 = t2->list();

    return getCompat(a1->elements, a2->elements);
  }
  if (t1->list() && t2->ptr())
  {
    auto l1 = t1->list();
    auto p2 = t2->ptr();

    return getCompat(l1->elements, p2->to);
  }
  if (t1->inst() && t2->inst())
  {
    auto i1 = t1->inst();
    auto i2 = t2->inst();
    return getCompat(i1->of, i2->of);
  }
  if (t1->mod() && t2->mod())
  {
    auto r1 = t1->mod()->ref;
    auto r2 = t2->mod()->ref;
    return getCompat(r1, r2);
  }
  if (second)
    return -1;
  else
    return getCompat(t2, t1, true);
}
int getCompatabilityScore(MethodSigStr &func,
                          MethodSigStr mock)
{
  // Namespace compat is presumably accounted for
  if (func.name->member->str() != mock.name->member->str())
  {
    return -1;
  }
  if (!func.isVariadic())
  {
    if (func.params.size() != mock.params.size())
    {
      return -1;
    }
  }

  // We got a generic function
  // Try to decipher each generic type
  if (func.generics.size())
  {
    if (func.generics.size() > 1)
      error("Functions may only have one generic parameter for the time being");
    if (mock.generics.size() > func.generics.size())
      return -1;
    if (mock.generics.size())
    {
      func.generics[0]->generic()->refersTo = mock.generics[0];
    }
    else
    {
      // find all the params that share the generic type
      // put them into a list, and use the getType method
      std::map<std::string, Type *> generic_type_mappings;
      for (unsigned int i = 0; i < mock.params.size(); i++)
      {
        auto fp = func.params[i];
        auto mp = mock.params[i];

        /*
        type_pair.first -> Found Type
        type_pair.second -> Found Against (Should be generic name)
        */
        auto type_pair = mp->type->find_mismatch(fp->type);
        if (!type_pair.second->generic())
          error("Expected A Generic Type");
        auto gen = type_pair.second->generic();
        auto as = type_pair.first;
        auto name = gen->name->str();
        if (!generic_type_mappings[name])
          generic_type_mappings[name] = as;
        else
        {
          // Get Compatable Type
          auto old_type = generic_type_mappings[name];
          auto new_type = getCommonType(old_type, as);
          generic_type_mappings[name] = new_type;
        }
      }

      // write the references according to the table
      for (auto generic_param : func.generics)
      {
        auto gen = generic_param->generic();
        auto name = gen->name->str();
        auto refersTo = generic_type_mappings[name];
        if (!refersTo)
          error("Failed To Get Type For Generic Param " + name);
        gen->refersTo = refersTo;
      }
    }
  }

  // Start with a base score, each infraction has a cost based on how
  // different it is
  int score = 0;
  for (unsigned int i = 0; i < mock.params.size(); i++)
  {
    auto baram = func.getParam(i)->type;
    auto taram = mock.getParam(i)->type;
    auto score = getCompat(baram, taram);
    if (score == -1)
      return -1;
    else
      score += 10 * score;
  }

  // TODO: Implement Type Reference Inheritance Tree Crawling (Each step up
  // the tree is +10)
  return score;
}

/**
 * Find the method of a module that best matches the call
 * this function respects access control, as well as overloads
 *
 */
MethodSignature *getMethodSig(Module *mod, MethodCall *call)
{
  if (call->ctx == nullptr)
    error("Cannot Resolve a contextless call");
  if (call->builtin())
    return nullptr;
  std::vector<Param *> testparams;
  int i = 0;
  for (auto p : call->params)
  {
    auto type = p->getType();
    if (type == nullptr)
    {
      Logger::error("Failed to get type for param " + call->name->str() + "[" + std::to_string(i) + "]");
      return nullptr;
    }
    testparams.push_back(new Param(type, nullptr));
    i++;
  }

  // Construct a fake signature to match against
  auto callsig = new MethodSignature;
  callsig->name = call->name;

  callsig->params = testparams;
  callsig->generics = call->make_generic_refs();

  // Make sure the modules match up

  auto sigstr = callsig->sigstr();
  Module *searches = call->name->mod->refersTo;
  if (searches == nullptr)
  {
    Logger::error("Failed to locate module " + call->name->mod->str());
    return nullptr;
  }
  // Run Compatibility Checks on each sigstr pair to find
  // the most compatible function to match to
  int best = -1;
  int idx = -1;
  i = -1;
  MethodSigStr bestSigStr;
  auto cs = callsig->sigstr();
  for (auto method : searches->getAllMethods())
  {
    i++;
    auto sig = method->sig;
    if (sig->nomangle)
      continue;
    auto sigs = sig->sigstr();
    int compat = getCompatabilityScore(sigs, cs);
    if (compat == -1)
      continue;
    if (compat <= best || best == -1)
    {
      best = compat;
      bestSigStr = sigs;
      idx = i;
    }
  }
  if (idx == -1)
  {
    Logger::error("Failed to generate function call to " + sigstr.str());
    return nullptr;
  }
  int ind = 0;
  for (auto method : searches->getAllMethods())
  {
    auto sig = method->sig;
    if (ind == idx)
    {
      if (sig->isGeneric())
      {

        auto gen = sig->impl_as_generic(bestSigStr.generics);
        return gen;
      }
      return sig;

    }
    ind++;
  }
  return nullptr;
}
Method* getMethod(Module* mod,  MethodCall* call){
  if(!mod)error("No mod?");
  if(!call)error("No Call?");
  Logger::debug("Get Method of " + mod->name->str());
  getMethodSig(mod, call);
  return nullptr;
  // return sigstr->belongsTo;
}

size_t getModuleMemberIdx(Module *mod, std::string name)
{
  size_t i = -1;
  for (auto prop : mod->getAllProperties())
  {
    if (!prop->instance_access)
      continue;
    i++;
    if (prop->name->member->str() == name)
      break;
  }
  return i;
}
