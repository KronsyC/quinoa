#pragma once
#include "./ast.hh"


class IfCond:public Statement{
public:
	Expression* cond;
	std::vector<Statement*> does;
	std::vector<Statement*> otherwise;

	IfCond(Expression* cond, std::vector<Statement*> does, std::vector<Statement*> otherwise){
		this->cond = cond;
		this->does = does;
		this->otherwise = otherwise;
	}

};

class WhileCond:public Statement, public SourceBlock{
public:
	Expression* cond;
	WhileCond(Expression* cond){
		this->cond = cond;
	}
	WhileCond() = default;

	std::vector<Statement*> flatten(){
		std::vector<Statement*> ret = {this};
		for(auto child:items){
			for(auto m:child->flatten())ret.push_back(m);
		}
		return ret;
	}
};


// Traditional for(init;cond;inc)
class ForRange: public Statement, public SourceBlock{
public:
	Expression* cond;
	SourceBlock* inc;

	ForRange( Expression* cond, SourceBlock* inc){
		this->cond = cond;
		this->inc = inc;
	}
	ForRange() = default;
	std::vector<Statement*> flatten(){
		std::vector<Statement*> ret = {this};
		for(auto child:items)for(auto m:child->flatten())ret.push_back(m);
		for(auto child:inc->items)for(auto m:child->flatten())ret.push_back(m);
		return ret;
	}
};