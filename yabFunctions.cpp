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
unsigned int fn::nextID;

/* function definitions */
void fn::dumpFunctionIDs()
{
	varNames << "Function IDs\n";
	auto i=functions.begin();
	if (i==functions.end())
	{
		varNames << "no named functions\n";
		return;
	}
	do
	{
		varNames << "Function " << i->first
			<< " has ID f" << i->second.get()->getID() << "\n";
		++i;
	} while (i!=functions.end());
}

void fn::generateOnNSub(expression *e, unsigned int skip)
{
	label *r=new label();
	output_cpp << "callStack=new subroutine(" << r->getID() << ");\n";
	label::generateOnNTo(e, skip);
	r->generate();
}

void fn::generateGosub(label *sub)
{
	label *r=new label();
	output_cpp << "callStack=new subroutine(" << r->getID() << ");\n";
	sub->generateJumpTo();
	r->generate();
}

fn *fn::getSub(string &name)
{
	auto iter=fn::functions.find(name);
	if(iter==fn::functions.end()) return nullptr;
	return iter->second.get();
}

variableType *fn::getLocalVar(string &name)
{
	auto iter=this->parameters.find(name);
	if (iter!=parameters.end()) return iter->second;
	auto i=locals.find(name);
	if (i!=locals.end()) return i->second.get();
	i=statics.find(name);
	if (i!=statics.end()) return i->second.get();
	return nullptr;
}

void fn::addParameter(string &name, enum TYPES t)
{
	variableType *v=new variableType(S_PARAMETER, name, t, this);
	v->generateBox(S_PARAMETER);
	this->parameters[name]=v;
	this->params.push_back(v);
}

/* TODO needs to be broken into smaller pieces */
operands *fn::generateCall(string &name, list<operands *>&paramList)
{
	static unsigned int callEnumerator;
	auto v=params.begin();
	operands *current;
	label *retAddr=new label();
	fn *g=fn::getSub(name);
	if (g==nullptr)
	{
		error(E_SUBROUTINE_NOT_FOUND);
	}
	if (paramList.size() > this->getNumParams())
	{
		error(E_TOO_MANY_PARAMETERS);
	}
	/* TODO CHECK THIS */
	++callEnumerator;
	heap_h << "struct f" << g->getID()
		<< " *sub" << callEnumerator << ";\n";
	output_cpp << " sub" << callEnumerator
		<< "= new f" << g->getID()
		<< "(" << retAddr->getID() << ");\n"
		<< "callStack = sub" << callEnumerator << ";\n";

	/* TODO Make parameter processing a separate function */
	while(paramList.size()>0)
	{
		current=paramList.front();
		paramList.pop_front();
		if(current->getSimpleVarType()!=(*v)->getType())
		{
			cerr << "assigning " << TYPENAMES[current->getType()]
				<< " to " << (*v)->getType() << endl;
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
	retAddr->generate();
	return g->rc;
}

/* typeless return for gosub family */
void fn::generateReturn()
{
	output_cpp << "state=subroutine::close();\nbreak;\n";
}

void fn::generateReturn(expression *expr)
{
	logger("evaluating expression");
	this->rc=expr->evaluate();
	logger("expression evaluated");
	this->kind=rc->getSimpleVarType();
	logger("generating return");
	output_cpp << "state=f" << this->getID()
		<< "::close();\nbreak;\n";
	switch (this->getType())
	{
		case T_UNKNOWNFUNC:
			break;
		case T_STRINGFUNC:
			if (kind!=T_STRINGVAR) error(E_TYPE_MISMATCH);
			break;
		case T_INTFUNC:
			if (kind!=T_INTVAR&&kind!=T_FLOATVAR) error(E_TYPE_MISMATCH);
			break;
		case T_FLOATFUNC:
			if(kind!=T_FLOATVAR) error(E_TYPE_MISMATCH);
			break;
		case T_GOSUB:
			error(E_GOSUB_CANNOT_RETURN_VALUE);
		default:
			error(E_BAD_SYNTAX);
	}
	logger("deleting expression");
	delete expr;
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
	if(DUMP)
	{
		auto i=locals.begin();
		varNames << "\nLocal variables in function f" << this->getID() << "\n";
		if(i==locals.end())
		{
			varNames << "no non-static locals" << endl;
		}
		else
		{
			do
			{
				varNames << "variable " << i->first	<< " has id v"
					<< i->second.get()->getID() << endl;
				++i;
			}while(i!=locals.end());
		}
		i=statics.begin();
		varNames << "\nStatic locals in function f" << this->getID() << "\n";
		if (i==statics.end())
		{
			varNames << "no static locals" << endl;
		}
		else
		{
			do
			{
				varNames << "variable " << i->first << " has id v"
					<< i->second.get()->getID() << endl;
				++i;
			} while (i!=statics.end());
		}
		auto iter=this->parameters.begin();
		if (iter==this->parameters.end())
		{
			varNames << "no parameters passed to function f" << this->getID() << endl;
		}
		else
		{
			do
			{
				varNames << "paramater " << iter->first << " has id v"
					<< iter->second->getID() << endl;
				++iter;
			} while (iter!=this->parameters.end());
			
		}
	}
	locals.clear();
	statics.clear();
	skipDef->generate();
	currentFunc=nullptr;
	scopeGlobal=true;
}

fn *fn::declare(string &s, enum CODES t, operands *returnCode)
{
	/* sd is the skipDef of this function */
	label *sd=new label();
	/* check for nesting error */
	if (!scopeGlobal) error(E_END_FUNCTION);
	/* check if this function name is already used */
	if (fn::functions.find(s)!=fn::functions.end()) error(E_DUPLICATE_SYMBOL);
	logger("declaration name cleared");
	sd->generateJumpTo();
	fn *self=new fn(t, returnCode);
	logger("fn allocated");
	fn::functions.insert({s,unique_ptr<fn>(self)});
	logger("fn inserted");
	/* initiate local scope */
	self->skipDef=sd;
	currentFunc=self;
	scopeGlobal=false;
	return self;
}

/* standard constructor for block-formatted functions */
fn::fn(enum CODES t, operands *returnCode)
{
	this->params.clear();
	this->type=t;
	this->id= ++nextID;
	/*define storage for locals*/
	if (t!=T_GOSUB)
	{
		funcs_h << "struct f" << this->id <<":public subroutine\n{\nf"
			<< this->id << "(unsigned int x):subroutine(x)\n{}\n"
			<< "virtual ~f" << this->id <<"()\n{}\n\n";
	}
	/*keep track of where the return code will be sent to*/
	this->rc=returnCode;
	/*allocate and generate start address label*/
	this->startAddr=new label();
	startAddr->generate();
}

/* Destructor is called only at the end of the unique pointer's existence */
fn::~fn()
{
	delete startAddr;
	delete skipDef;
}
