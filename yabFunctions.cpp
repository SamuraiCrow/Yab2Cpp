/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

/* static initializers */
unordered_map<string, unique_ptr<fn> > fn::functions;
list<fn *>fn::callStack;
unsigned int fn::nextID=0;

/* function definitions */
void fn::dumpCallStack()
{
	auto i=callStack.rbegin();
	if (i==callStack.rend())
	{
		logfile << "call stack was empty\n";
		return;
	}
	do
	{
		logfile << (*i)->funcName << "\n";
		++i;
	} while(i!=callStack.rend());
}

fn *fn::getCurrentSub()
{
	return callStack.back();
}

void fn::generateOnNSub(expression *e, unsigned int skip)
{
	label *r=new label();
	fn *self=new fn(r);
	fn::callStack.push_back(self);
	label::generateOnNTo(e, skip);
	r->generate();
}

void fn::generateGosub(label *sub)
{
	label *r=new label();
	fn *self=new fn(r);
	fn::callStack.push_back(self);
	sub->generateJumpTo();
	r->generate();
}

fn::fn(label *gosub):codeType(T_GOSUB)
{
	this->funcName="unnamed gosub";
	this->ret=gosub;
}

fn *fn::getSub(string &name)
{
	auto iter=fn::functions.find(name);
	if(iter==fn::functions.end()) return nullptr;
	return iter->second.get();
}

operands *fn::generateCall(string &name, list<operands *>&paramList)
{
	auto v=params.begin();
	operands *current;
	fn *g=fn::getSub(name);
	if (g==nullptr)
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

	/* TODO Make parameter processing a separate function */
	while(paramList.size()>0)
	{
		current= paramList.front();
		paramList.pop_front();
		if(current->getSimpleVarType()!=(*v)->getType())
		{
			error(E_TYPE_MISMATCH);
		}
		(*v)->assignment(new expression(current));
		++v;
	}
	/* pad remaining unassigned variables with empty values */
	while (v!=params.end())
	{
		switch ((*v)->getType())
		{
			case T_FLOATVAR:
				(*v)->assignment(new expression(new constOp("0.0", T_FLOAT)));
				break;
			case T_INTVAR:
				(*v)->assignment(new expression(new constOp("0", T_INT)));
				break;
			case T_STRINGVAR:
				(*v)->assignment(new expression(new constOp("", T_STRING)));
			default:
				error(E_INTERNAL);
		}
		++v;
	}
	g->startAddr->generateJumpTo();
	g->ret->generate();
	return g->rc;
}

void fn::generateReturn()
{
	fn *c=getCurrentSub();
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
			error(E_TYPE_MISMATCH);
	}
}

void fn::generateReturn(expression *expr)
{
	this->rc=expr->evaluate();
	this->kind=rc->getSimpleVarType();
	this->ret->generateJumpTo();
	fn::callStack.pop_back();
	delete expr;
	switch (this->getType())
	{
		case T_UNKNOWNFUNC:
			return;
		case T_STRINGFUNC:
			if (kind!=T_STRINGVAR) error(E_TYPE_MISMATCH);
			return;
		case T_INTFUNC:
			if (kind!=T_INTVAR&&kind!=T_FLOATVAR) error(E_TYPE_MISMATCH);
			return;
		case T_FLOATFUNC:
			if(kind!=T_FLOATVAR) error(E_TYPE_MISMATCH);
			return;
		case T_GOSUB:
			error(E_GOSUB_CANNOT_RETURN_VALUE);
		default:
			error(E_BAD_SYNTAX);
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
	locals.clear();
	statics.clear();
	this->params.clear();
	scopeGlobal=true;
}

fn::fn(string &s, enum CODES t, operands *returnCode):codeType(t)
{
	/*check for nesting error */
	if (!scopeGlobal) error(E_END_FUNCTION);
	/*check if this function name is already used*/
	if (fn::functions.find(s)!=fn::functions.end()) error(E_DUPLICATE_SYMBOL);
	this->funcName=s;
	this->id= ++nextID;
	/*define storage for locals*/
	funcs_h << "struct f" << this->id <<"\n{\n";
	/*define label space for return*/
	this->ret=new label();
	/*allocate function name*/
	fn::functions[s]=unique_ptr<fn>(this);
	/* initiate local scope */
	scopeGlobal=false;
	/*keep track of where the return code will be sent to*/
	this->rc=returnCode;
	/*allocate and generate start address label*/
	this->startAddr=new label();
	startAddr->generate();
}
