#pragma once
#include "./ast.hh"


class IfCond:public Statement, public SourceBlock{
public:
	Expression* cond;
	SourceBlock* does = nullptr;
	SourceBlock* otherwise = nullptr;

	bool returns(){
		if(does==nullptr || otherwise == nullptr)return false;
		return does->returns() && otherwise->returns();
	}

	IfCond(Expression* cond, SourceBlock* does, SourceBlock* otherwise=nullptr){
		this->cond = cond;
		this->does = does;
		this->otherwise = otherwise;
	}
	IfCond() = default;
	std::vector<Statement*> flatten(){
		std::vector<Statement*> ret = {this};
		bool hasElse = (this->otherwise) != nullptr;
		if(hasElse){
			auto flat = otherwise->flatten();
			for(auto m:otherwise->flatten())ret.push_back(m);
		}

		for(auto m:does->flatten())ret.push_back(m);
		for(auto m:cond->flatten())ret.push_back(m);
		return ret;
	}

};

class WhileCond:public Statement, public SourceBlock{
public:
	Expression* cond;
	WhileCond(Expression* cond){
		this->cond = cond;
	}
	WhileCond() = default;
	bool returns(){
		// we cant know if a while loop will definitely return
		return false;
	}
	std::vector<Statement*> flatten(){
		std::vector<Statement*> ret = {this};
		for(auto child:items){
			for(auto m:child->flatten())ret.push_back(m);
		}
		for(auto m:cond->flatten())ret.push_back(m);
		return ret;
	}
};


// Traditional for(init;cond;inc)
class ForRange: public Statement, public SourceBlock{
public:
	Expression* cond;
	SourceBlock* inc;
	bool returns(){
		return false;
	}
	ForRange( Expression* cond, SourceBlock* inc){
		this->cond = cond;
		this->inc = inc;
	}
	ForRange() = default;
	std::vector<Statement*> flatten(){
		std::vector<Statement*> ret = {this};
		for(auto child:items)for(auto m:child->flatten())ret.push_back(m);
		for(auto child:inc->flatten())ret.push_back(child);
		return ret;
	}
};