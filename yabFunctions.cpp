/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

/* function definitions */
fn *fn::getCurrentSub()
{
	return callStack.back;
}

void fn::generateOnNSub(expression *e)
{
	shared_ptr<label>r=new label();
	shared_ptr<fn> self=new fn(r);
	fn::callStack.push_back(self);
	label::generateOnNTo(e);
	r->generate();
}

void fn::generateGosub(shared_ptr<label> sub)
{
	shared_ptr<label>r=new label();
	shared_ptr<fn> self=new fn(r);
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
	auto iter=fn::functions.find(name)
	if(iter==fn::functions.end()) return NULL;
	return iter->second;
}

operands *fn::generateCall(string &name, list<shared_ptr<operands> >&paramList)
{
	auto v=params.begin();
	shared_ptr<operands>current;
	shared_ptr<fn>g=fn::getSub(name);
	if (g==NULL)
	{
		errorLevel=E_SUBROUTINE_NOT_FOUND;
		exit(1);
	}
	if (paramList.size()>params.size())
	{
		errorLevel=E_TOO_MANY_PARAMETERS;
		exit(1);
	}
	output_cpp << "struct *f" << /* TODO: finish this */ 
		<< "= new struct f" << g->getID();
	while(paramList.size()>0)
	{
		current=paramList.front;
		paramList.pop_front();
		if(current->getSimpleVarType()!=*v->getType())
		{
			errorLevel=E_TYPE_MISMATCH;
			exit(1);
		}
		*v->assignment(new expression(current));
		++v;
	}

	return g->/*TODO FINISH THIS*/
}

void fn::generateReturn()
{
	shared_ptr<fn>c=fn::getCurrent();
	switch(c->getType())
	{
		case T_UNKNOWNFUNC:
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

operands *fn::generateReturn(expression *expr)
{
	operands *out=expr->evaluate();
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
		this->generateReturn()
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
	if (fn::functions->find(s)!=fn::functions.end()) error(E_DUPLICATE_SYMBOL);
	this->id= ++nextID;
	funcs_h << "struct f" << this->id <<"\n{\n";
	this->ret=new label();
	fn::functions[s]=this;
	scopeGlobal=false;
}
