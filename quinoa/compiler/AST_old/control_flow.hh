#pragma once
#include "./ast.hh"

class IfCond : public Statement
{
public:
    Expression* cond;
    SourceBlock* does;
    SourceBlock* otherwise = nullptr;

    IfCond(Expression* cond, SourceBlock* does, SourceBlock* otherwise = nullptr)
    {
	if(instanceof <IfCond>(otherwise)) {
	    auto cond = (IfCond*)otherwise;
	    this->otherwise = new SourceBlock(cond);
	} else
	    this->otherwise = otherwise;
	this->cond = cond;
	this->does = does;
    }
    IfCond() = default;
    std::vector<Statement*> flatten()
    {
	std::vector<Statement*> ret = { this };
	if(this->otherwise) {
	    auto flat = otherwise->flatten();
	    for(auto m : otherwise->flatten())
		ret.push_back(m);
	}

	for(auto m : does->flatten())
	    ret.push_back(m);
	for(auto m : cond->flatten())
	    ret.push_back(m);
	return ret;
    }
    IfCond* copy(SourceBlock* ctx)
    {
	auto ifc = new IfCond;
	ifc->ctx = ctx;
	ifc->cond = cond->copy(ctx);
	ifc->does = does->copy(ctx);
	if(otherwise)
	    ifc->otherwise = otherwise->copy(ctx);
	return ifc;
    }
};

class WhileCond : public SourceBlock
{
public:
    Expression* cond;
    WhileCond(Expression* cond)
    {
	this->cond = cond;
    }
    WhileCond() = default;
    bool returns()
    {
	// we cant know if a while loop will definitely return
	return false;
    }
    std::vector<Statement*> flatten()
    {
	std::vector<Statement*> ret = { this };
	for(auto child : *this) {
	    for(auto m : child->flatten())
		ret.push_back(m);
	}
	for(auto m : cond->flatten())
	    ret.push_back(m);
	return ret;
    }
};

// Traditional for(init;cond;inc)
class ForRange : public SourceBlock
{
public:
    Expression* cond;
    SourceBlock* inc;
    bool returns()
    {
	return false;
    }
    ForRange(Expression* cond, SourceBlock* inc)
    {
	this->cond = cond;
	this->inc = inc;
    }
    ForRange() = default;
    std::vector<Statement*> flatten()
    {
	std::vector<Statement*> ret = { this };
	for(auto child : *this)
	    for(auto m : child->flatten())
		ret.push_back(m);
	for(auto child : inc->flatten())
	    ret.push_back(child);
	return ret;
    }
};