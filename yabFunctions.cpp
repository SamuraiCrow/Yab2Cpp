/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

class label;

/* function definitions */
shared_ptr<fn>fn::getCurrentSub()
{
	return callStack.back();
}

void fn::generateOnNSub(shared_ptr<expression>e, unsigned int skip)
{
	shared_ptr<label>r=shared_ptr<label>(new label());
	shared_ptr<fn> self=shared_ptr<fn>(new fn(r));
	fn::callStack.push_back(self);
	label::generateOnNTo(e, skip);
	r->generate();
}

void fn::generateGosub(shared_ptr<label> sub)
{
	shared_ptr<label>r=shared_ptr<label>(new label());
	shared_ptr<fn> self=shared_ptr<fn>(new fn(r));
	fn::callStack.push_back(self);
	sub->generateJumpTo();
	r->generate();
}

fn::fn(shared_ptr<label>gosub):codeType(T_GOSUB)
{
	this->ret=gosub;
}

shared_ptr<fn> fn::getSub(string &name)
{
	auto iter=fn::functions.find(name);
	if(iter==fn::functions.end()) return NULL;
	return iter->second;
}

shared_ptr<operands>fn::generateCall(string &name,
	list<shared_ptr<operands> >paramList)
{
	auto v=params.begin();
	shared_ptr<operands>current;
	shared_ptr<fn>g=fn::getSub(name);
	if (g==NULL)
	{
		error(E_SUBROUTINE_NOT_FOUND);
	}
	if (paramList.size()>params.size())
	{
		error(E_TOO_MANY_PARAMETERS);
	}
	/* TODO CHECK THIS */
	output_cpp << "struct f" << g->getID()
		<< "*sub" << this->getID()
		<< "= new struct f" << g->getID()
		<< "();\n";
	while(paramList.size()>0)
	{
		current= paramList.front();
		paramList.pop_front();
		if(current->getSimpleVarType()!=(*v)->getType())
		{
			error(E_TYPE_MISMATCH);
		}
		(*v)->assignment(shared_ptr<expression>(
			new expression(current)));
		++v;
	}
	/* pad remaining unassigned variables with empty values */
	while (v!=params.end())
	{
		switch ((*v)->getType())
		{
			case T_FLOATVAR:
				(*v)->assignment(shared_ptr<expression>(
					new expression(
						operands::createConst("0.0", T_FLOAT)
					)));
				break;
			case T_INTVAR:
				(*v)->assignment(shared_ptr<expression>(
					new expression(
						operands::createConst("0", T_INT)
					)));
				break;
			case T_STRINGVAR:
				(*v)->assignment(shared_ptr<expression>(
					new expression(
						operands::getOrCreateStr(string("")
					))));
			default:
				error(E_TYPE_MISMATCH);
		}
		++v;
	}
	return g->/*TODO FINISH THIS*/
}

void fn::generateReturn()
{
	shared_ptr<fn>c=getCurrentSub();
	switch(c->getType())
	{
		case T_UNKNOWNFUNC:
			/* set return type to NONE */
			this->kind=T_NONE;
			/*fallthrough*/
		case T_GOSUB:
			c->ret->generateJumpTo();
			fn::callStack.pop_back();
			break;
		default:
			errorLevel=E_TYPE_MISMATCH;
			exit(1);
	}
}

shared_ptr<operands>fn::generateReturn(shared_ptr<expression>expr)
{
	shared_ptr<operands>out=expr->evaluate();
	this->kind=out->getSimpleVarType();
	this->ret->generateJumpTo();
	fn::callStack.pop_back();
	switch (this->getType())
	{
		case T_UNKNOWNFUNC:
			return out;
		case T_STRINGFUNC:
			if (kind!=T_STRINGVAR)
			{
				errorLevel=E_TYPE_MISMATCH;
				exit(1);
			}
			return out;
		case T_INTFUNC:
			if (kind!=T_INTVAR&&kind!=T_FLOATVAR)
			{
				errorLevel=E_TYPE_MISMATCH;
				exit(1);
			}
			return out;
		case T_FLOATFUNC:
			if(kind!=T_FLOATVAR)
			{
				errorLevel=E_TYPE_MISMATCH;
				exit(1);
			}
			return out;
		case T_GOSUB:
			{
				errorLevel=E_GOSUB_CANNOT_RETURN_VALUE;
				exit(1);
			}
		default:
			{
				errorLevel=E_BAD_SYNTAX;
				exit(1);
			}
	}
}

/* not allowed in function definitions directly */
void fn::generateBreak()
{
	error(E_BAD_SYNTAX);
}

void fn::close()
{
	/* check if no returns and no return type */
	enum CODES t=this->getType();
	if (this->kind==T_UNKNOWN&&
		(t==T_UNKNOWNFUNC||t==T_VOIDFUNC))
	{
		/* generate a typeless return */
		this->generateReturn();
	}
	funcs_h << "};\n";
	fn::locals.clear();
	fn::statics.clear();
	this->params.clear();
	scopeGlobal=true;
}

fn::fn(string &s, enum CODES t):codeType(t)
{
	if (!scopeGlobal) error(E_END_FUNCTION);
	if (fn::functions.find(s)!=fn::functions.end()) error(E_DUPLICATE_SYMBOL);
	this->id= ++nextID;
	funcs_h << "struct f" << this->id <<"\n{\n";
	this->ret=shared_ptr<label>(new label());
	fn::functions[s]=this;
	scopeGlobal=false;
}
