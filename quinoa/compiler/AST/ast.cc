#include "./ast.hh"


Type *getCommonType(Type *t1, Type *t2)
{
    if (t1 == nullptr || t2 == nullptr)
      error("one of the types is null");
    if (t1 == t2)
      return t1;

    if(t1->primitive() && t2->primitive()){
      auto t1p = t1->primitive();
      auto t2p = t2->primitive();
      return t1p->getMutual(t2p);
    }
    if(t1->list() && t2->list()){
      auto t1l = t1->list()->elements;
      auto t2l = t2->list()->elements;
      auto mut = getCommonType(t1l, t2l);
      return ListType::get(mut);
    }
    error("Failed To Get Common Type Between " + t1->str() + " and " + t2->str());
    return nullptr;
}