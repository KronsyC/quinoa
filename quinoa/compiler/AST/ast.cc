#include "./ast.hh"

Type *getCommonType(Type *_t1, Type *_t2)
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
  error("Failed To Get Common Type Between " + t1->str() + " and " + t2->str(), true);
  return nullptr;
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