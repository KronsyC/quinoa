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

class WhileCond:public Statement{
public:
	Expression* cond;
	std::vector<Statement*> exec;

	WhileCond(Expression* cond, std::vector<Statement*> exec){
		this->cond = cond;
		this->exec = exec;
	}
};


// Traditional for(init;cond;inc)
class ForRange: public Statement{
public:
	Statement* init;
	Expression* cond;
	Statement* inc;
	std::vector<Statement*> exec;

	ForRange(Statement* init, Expression* cond, Statement* inc, std::vector<Statement*> exec){
		this->init = init;
		this->cond = cond;
		this->inc = inc;
		this->exec = exec;
	}
};