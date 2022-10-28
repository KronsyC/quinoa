#include "./ast.hh"

Type *getCommonType(Type *_t1, Type *_t2, bool second_pass)
{
  auto t1 = _t1->drill();
  auto t2 = _t2->drill();
  if (t1 == nullptr || t2 == nullptr)
    error("one of the types is null", true);
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
  if(t1->ptr() && t2->list()){
    auto t1p = t1->ptr();
    auto t2l = t2->list();
    auto mem = getCommonType(t1p->to, t2l->elements);
    return new TPtr(mem);
  }
  if(second_pass)
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

llvm::StructType* structify(Module* mod){

  
  std::vector<llvm::Type*> struct_elements;

  auto llmod = builder()->GetInsertBlock()->getModule();
  auto struct_name = "struct_"+mod->fullname()->str();
  for(auto struct_type:llmod->getIdentifiedStructTypes()){
    if(struct_type->getName().str() == struct_name)return struct_type;
  }
  for(auto prop:mod->getAllProperties()){
    if(!prop->instance_access)continue;
    auto prop_type = prop->type->getLLType();
    struct_elements.push_back(prop_type);
  }
  auto struct_type = llvm::StructType::create(*llctx(), struct_elements, struct_name);
  return struct_type;
}
Property* getProperty(Module* mod, std::string propname){
  for(auto p:mod->getAllProperties()){
    if(p->name->member->str() == propname)return p;
  }
  return nullptr;
}

size_t getModuleMemberIdx(Module* mod, std::string name){
  size_t i = -1;
  for(auto prop:mod->getAllProperties()){
    if(!prop->instance_access)continue;
    i++;
    if(prop->name->member->str() == name)break;
  }
  return i;
}
