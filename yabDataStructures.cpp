/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

class fn;

/* scope methods */
ofstream &scope::operator<<(ostream &in)
{
	switch (this->myscope)
	{
		case S_LOCAL:
			return funcs_h;
		case S_GLOBAL:
		case S_STATIC:
			return heap_h;
	}
}

/* methods for operands */
shared_ptr<operands>operands::findGlobal(string &s)
{
	auto iter=operands::globals.find(s);
	if (iter==operands::globals.end())
	{
		return NULL;
	}
	return iter->second;
}

void operands::dumpVars()
{
	varNames << "Global Variables\n";
	for(auto iter=globals.begin(); iter!=globals.end(); ++iter)
	{
		varNames << "variable " << iter->first << " has ID " << iter->second << "\n";
	}
	varNames << endl;
}

enum TYPES operands::getSimpleVarType()
{
	switch (this->getType())
	{
		case T_FLOAT:
		case T_FLOATCALL_ARRAY:
		case T_FLOATVAR:
			return T_FLOATVAR;
		case T_INT:
		case T_INTCALL_ARRAY:
		case T_INTVAR:
			return T_INTVAR;
		case T_STRING:
		case T_STRINGCALL_ARRAY:
		case T_STRINGVAR:
			return T_STRINGVAR;
	}
	error(E_UNASSIGNABLE_TYPE);
}

/* operands used by expression parser and variables */

operands::operands(enum TYPES t)
{
	this->id = ++nextID;
	this->type=t;
}

void operands::generateBox(enum SCOPES s)
{
	string x;
	scope y(s);
	stringstream ss;
	switch (this->getSimpleVarType())
	{
	case T_INTVAR:
		ss << "int v";
		break;
	case T_FLOATVAR:
		ss << "double v";
		break;
	case T_STRINGVAR:
		ss << "string v";
		break;
	default:
		error(E_TYPE_MISMATCH);
	}
	ss << this->getID() << ";\n";
	ss.str(x);
	y << x;
}

shared_ptr<operands>operands::getOrCreateGlobal(string &s, enum TYPES t)
{
	auto op=globals.find(s);
	if (op==globals.end())
	{
		return shared_ptr<variable>(new variable(S_GLOBAL, s, t));
	}
	return op->second;
}

string operands::boxName()
{
	ostringstream s;
	string x;
	switch (this->getType())
	{
	case T_STRINGVAR:
	case T_INTVAR:
	case T_FLOATVAR:
		s << 'v' << this->getID();
		s.str(x);
		s.clear();
		return x;
		break;
	
	default:
		error(E_INTERNAL);
	}
}

void constOp::processConst(unsigned int i)
{
	stringstream me;
	me << 'k' << i;
	me.str(box);
}

void constOp::processConst( const string &s)
{
	processConst(getID());
	consts_h << box << "=" << s << ";\n";
}


/* constructor for constOp */
constOp::constOp(const string &s, enum TYPES t):operands(t)
{
	switch (t)
	{
		case T_INT:
			consts_h << "const int ";
			processConst(s);
			break;
		case T_FLOAT:
			consts_h << "const double ";
			processConst(s);
			break;
		case T_STRING:
			{
				auto i=strConst.find(s);
				if (i!=strConst.end())
				{
					processConst((*i).second);
				}
				else
				{
					consts_h << "const string ";
					processConst(s);
					strConst[s]=getID();
				}
			}
			break;
		default:
			error(E_TYPE_MISMATCH);
			break;
	}
}

/* expression parsing routines */

/* binary vs. unary ops */
bool expression::isBinOp()
{
	switch (this->getOp())
	{
		case O_NEGATE:
		case O_NOT:
		case O_INVERT:
		case O_INT_TO_FLOAT:
			return false;
			break;
	}
	return true;
}

shared_ptr<operands>expression::evaluate()
{
	if (this->getOp()==O_TERM) return op;
	shared_ptr<operands>l;
	shared_ptr<operands>r;
	enum TYPES t;
	enum SCOPES scopeVar=(scopeGlobal?S_GLOBAL:S_LOCAL);
	l=this->getLeft()->evaluate();
	if (this->isBinOp())
	{
		r=this->getRight()->evaluate();
		enum TYPES lt=l->getSimpleVarType();
		enum TYPES rt=r->getSimpleVarType();
		if (lt==T_INTVAR && rt==T_FLOATVAR)
		{
			l=shared_ptr<operands>((new expression(shared_ptr<expression>(new expression(l.get())), O_INT_TO_FLOAT))->evaluate());
			lt=T_FLOATVAR;
		}
		if (lt==T_FLOATVAR && rt==T_INTVAR)
		{
			r=shared_ptr<operands>((new expression(shared_ptr<expression>(new expression(r.get())), O_INT_TO_FLOAT))->evaluate());
			rt=T_FLOATVAR;
		}
		if (lt!=rt)error(E_TYPE_MISMATCH);
		t=lt;
	}
	else
	{
		t=l->getSimpleVarType();
		r=NULL;
	}
	if (t==T_STRINGVAR) return expression::stringEval(l, r);
	switch (this->getOp())
	{
	case O_INVERT:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "= ~" << l->boxName() << ";\n";
		break;
	case O_NEGATE:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "= -" << l->boxName() << ";\n";
		break;
	case O_NOT:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "= !" << l->boxName() << ";\n";
		break;
	case O_INT_TO_FLOAT: /*Note: this duplicates functionality of variable assignment */
		this->op=shared_ptr<operands>(new operands(T_FLOATVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "= const_cast<double>(" 
			<< l->boxName() << ");\n";
		/* TODO:  Check for divide by zero error and modulo zero error */
	case O_REMAINDER:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "%" << r->boxName() << ";\n";
		break;
	case O_DIVIDE:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "/" << r->boxName() << ";\n";
		break;
	case O_PLUS:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "+" << r->boxName() << ";\n";
		break;
	case O_MINUS:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "-" << r->boxName() << ";\n";
		break;
	case O_MULTIPLY:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "*" << r->boxName() << ";\n";
		break;
	case O_OR:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "|" << r->boxName() << ";\n";
		break;
	case O_AND:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "&" << r->boxName() << ";\n";
		break;
	case O_GREATER:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << ">" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "<" << r->boxName() << ")?-1:0;\n";
		break;
	case O_GREATER_EQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << ">=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS_EQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "<=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_EQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "==" << r->boxName() << ")?-1:0;\n";
		break;
	case O_UNEQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "!=" << r->boxName() << ")?-1:0;\n";
		break;
	default:
		errorLevel=E_INTERNAL;
		exit(1);
		break;
	}
	/* convert expression into single operand */
	this->oper=O_TERM;
	return this->op;
}

shared_ptr<operands> expression::stringEval(shared_ptr<operands>l, shared_ptr<operands>r)
{
	if (this->getOp()==O_STRING_CONCAT)
	{
		this->op=shared_ptr<operands>(new operands(T_STRINGVAR));
		this->op->generateBox(scopeGlobal?S_GLOBAL:S_LOCAL);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "+" << r->boxName();
	}
	else error(E_INTERNAL);
	/* convert expression into single operand */
	this->oper=O_TERM;
	return this->op;
}

/* variable definitions */
variable::variable(enum SCOPES s, string &name, enum TYPES t):operands(t)
{
	this->myScope=s;
	switch (s)
	{
		case S_LOCAL:
			if(fn::locals.find(name)!=fn::locals.end() ||
				fn::statics.find(name)!=fn::statics.end() ) error(E_DUPLICATE_SYMBOL);
			fn::locals[name]=shared_ptr<variable>(this);
			break;		
		case S_GLOBAL:
			if(findGlobal(name)!=NULL) error(E_DUPLICATE_SYMBOL);
			globals[name]=shared_ptr<variable>(this);
			break;
		case S_STATIC:
			if(fn::locals.find(name)!=fn::locals.end() ||
				fn::statics.find(name)!=fn::statics.end() ) error(E_DUPLICATE_SYMBOL);
			fn::statics[name]=shared_ptr<variable>(this);
			break;
		default:
			error(E_INTERNAL);
	}
	this->generateBox(s);
}

shared_ptr<variable> variable::getOrCreateVar(string &name, enum TYPES t)
{
	if (!scopeGlobal)
	{
		return fn::getOrCreateVar(t, name, false);
	}
	/* TODO: verify if type is compatible */
	shared_ptr<operands>op=operands::getOrCreateGlobal(name, t);
	shared_ptr<variable>v=shared_ptr<variable>(new variable());
	v->assignment(shared_ptr<expression>(new expression(op.get())));
	return v;
}

void variable::assignment(shared_ptr<expression>value)
{
	shared_ptr<operands>op=value->evaluate();
	enum TYPES t=op->getSimpleVarType();
	switch (this->getType())
	{
		case T_FLOATVAR:
			if (t==T_INTVAR)
			{
				output_cpp << this->boxName() << "="
					<< "static_cast<double>("
					<< op->boxName() << ");\n";
			}
			else
			{
				if (t!=T_FLOATVAR) error(E_TYPE_MISMATCH);
			}
			output_cpp << this->boxName() << "="
				<< op->boxName() << ";\n";
			break;
		default:
			if (t!=this->getType()) error(E_TYPE_MISMATCH);
			output_cpp << this->boxName() << "="
				<< op->boxName() << ";\n";
			break;
	}
}
