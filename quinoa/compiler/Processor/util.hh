#pragma once
#include <vector>
#include "./processor.h"
using namespace std;
// Flattens a source tree as a single list of all of its members
// and their children bubbled up to the top-level
// useful for iterating over every single node of a given type
vector<Statement *> flatten(Block<Statement> stmnt) {
  vector<Statement *> retval;
  for (auto item : stmnt.items) {
    if (instanceof <Block<Statement>>(item)) {
      auto nested = (Block<Statement> *)item;
      auto sub = flatten(*nested);
      for (auto c : sub)
        retval.push_back(c);

    } else if (instanceof <Statement>(item)) {
      auto stmnt = (Statement *)item;
      auto flat = stmnt->flatten();
      for (auto i : flat)
        retval.push_back(i);
    }
  }
  return retval;
}