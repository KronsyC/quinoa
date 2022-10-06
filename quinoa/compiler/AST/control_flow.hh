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

class WhileCond:public Statement, SourceBlock{
public:
	Expression* cond;

	WhileCond(Expression* cond, std::vector<Statement*> exec){
		this->cond = cond;
		this->items = exec;
	}
};


// Traditional for(init;cond;inc)
class ForRange: public Statement, SourceBlock{
public:
	Statement* init;
	Expression* cond;
	Statement* inc;

	ForRange(Statement* init, Expression* cond, Statement* inc, std::vector<Statement*> exec){
		this->init = init;
		this->cond = cond;
		this->inc = inc;
		this->items = exec;
	}
};