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

shared_ptr<operands> operands::getOrCreateStr(string s)
{
	auto iter=operands::strConst.find(s);
	if (iter!=operands::strConst.end())
	{
		return shared_ptr<operands>(new operands(iter->second, T_STRING));
	}
	++nextID;
	consts_h << "const string k" << nextID << "=\"" << s << "\";\n";
	operands::strConst[s]=nextID;
	return shared_ptr<operands>(new operands(nextID, T_STRING));
}

shared_ptr<operands>operands::createConst(string s, enum TYPES t)
{
	shared_ptr<operands>me=shared_ptr<operands>(new operands(t));
	if (t==T_INT)
	{
		consts_h << "const int k";
	}
	else
	{
		if (t==T_FLOAT)
		{
			consts_h << "const double k";
		}
		else
		{
			errorLevel=E_TYPE_MISMATCH;
			exit(1);
		}
	}
	consts_h << me->getID() << "=" << s << ";\n";
	return me;
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

void operands::generateBox(ostream &scope)
{
	switch (this->getSimpleVarType())
	{
	case T_INTVAR:
		scope << "int v";
		break;
	case T_FLOATVAR:
		scope << "double v";
		break;
	case T_STRINGVAR:
		scope << "string v";
		break;
	default:
		errorLevel=E_TYPE_MISMATCH;
		exit(1);
		break;
	}
	scope << this->getID() << ";\n";
}

shared_ptr<operands>operands::getOrCreateGlobal(string &s, enum TYPES t)
{
	auto op=globals.find(s);
	if (op==globals.end())
	{
		return shared_ptr<variable>(new variable(heap_h, s, t));
	}
	return op->second;
}

string operands::boxName()
{
	ostringstream s;
	switch (this->getType())
	{
	case T_STRINGVAR:
	case T_INTVAR:
	case T_FLOATVAR:
		s << 'v' << this->getID();
		return s.str();
		break;
	
	default:
		error(E_INTERNAL);
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
	ostream &scope=scopeGlobal?heap_h:funcs_h;
	l=this->getLeft()->evaluate();
	if (this->isBinOp())
	{
		r=this->getRight()->evaluate();
		enum TYPES lt=l->getSimpleVarType();
		enum TYPES rt=r->getSimpleVarType();
		if (lt==T_INTVAR && rt==T_FLOATVAR)
		{
			l=shared_ptr<operands>((new expression(shared_ptr<expression>(new expression(l)), O_INT_TO_FLOAT))->evaluate());
			lt=T_FLOATVAR;
		}
		if (lt==T_FLOATVAR && rt==T_INTVAR)
		{
			r=shared_ptr<operands>((new expression(shared_ptr<expression>(new expression(r)), O_INT_TO_FLOAT))->evaluate());
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
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "= ~" << l->boxName() << ";\n";
		break;
	case O_NEGATE:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "= -" << l->boxName() << ";\n";
		break;
	case O_NOT:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "= !" << l->boxName() << ";\n";
		break;
	case O_INT_TO_FLOAT: /*Note: this duplicates functionality of variable assignment */
		this->op=shared_ptr<operands>(new operands(T_FLOATVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "= const_cast<double>(" 
			<< l->boxName() << ");\n";
		/* TODO:  Check for divide by zero error and modulo zero error */
	case O_REMAINDER:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "%" << r->boxName() << ";\n";
		break;
	case O_DIVIDE:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "/" << r->boxName() << ";\n";
		break;
	case O_PLUS:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "+" << r->boxName() << ";\n";
		break;
	case O_MINUS:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "-" << r->boxName() << ";\n";
		break;
	case O_MULTIPLY:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "*" << r->boxName() << ";\n";
		break;
	case O_OR:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "|" << r->boxName() << ";\n";
		break;
	case O_AND:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "&" << r->boxName() << ";\n";
		break;
	case O_GREATER:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << ">" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "<" << r->boxName() << ")?-1:0;\n";
		break;
	case O_GREATER_EQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << ">=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS_EQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "<=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_EQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "==" << r->boxName() << ")?-1:0;\n";
		break;
	case O_UNEQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scope);
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
		this->op->generateBox(scopeGlobal?heap_h:funcs_h);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "+" << r->boxName();
	}
	else error(E_INTERNAL);
	/* convert expression into single operand */
	this->oper=O_TERM;
	return this->op;
}

/* variable definitions */
variable::variable(ostream &scope, string &name, enum TYPES t):operands(t)
{
	this->generateBox(scope); /*TODO:  FINISH THIS*/
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
	v->assignment(shared_ptr<expression>(new expression(op)));
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
