/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

/* methods for operands */
static operands *findGlobal(string &s)
{
	auto iter=operands::globals.find(s);
	if (iter==operands::globals.end())
	{
		return NULL;
	}
	return iter->second;
}

static void operands::dumpVars()
{
	varNames << "Global Variables\n";
	for(auto iter=globals.begin(); iter!=globals.end(); ++iter)
	{
		varNames << "variable " << iter->first << " has ID " << iter->second << "\n";
	}
	varNames << endl;
}

unsigned int operands::getOrCreateStr(string &s)
{
	auto iter=constStr.find(s);
	if (iter!=constStr.end()) return iter->second;
	++nextID;
	consts_h << "const string k" << nextID << "=\"" << s << "\";\n";
	constStr[s]=nextID;
	return nextID;
}

unsigned int operands::createConst(string &s, enum TYPES t)
{
	operands *me=new operands(t);
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
	switch type
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

operands *operands::getOrCreateGlobal(string &s, enum TYPES t)
{
	operands op*=operands::globals->find(s);
	if (op==globals.end())
	{
		op=new variable(heap_h, s, t);
	}
	return op;
}

void operands::boxName(ostream &scope)
{
	switch (this->getType())
	{
	case T_STRINGVAR:
	case T_INTVAR:
	case T_FLOATVAR:
		break;
	
	default:
		error(E_INTERNAL);
	}
	scope << "v" << this->getID();
}

/* expression parsing routines */

/* binary vs. unary ops */
bool expression::isBinOp()
{
	switch this->getOp()
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

operands *expression::evaluate()
{
	if (this->getOp()==O_TERM) return op;
	operands *l, *r;
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
			l=new expression(new expression(l), O_INT_TO_FLOAT)->evaluate();
			lt=T_FLOATVAR;
		}
		if (lt==T_FLOATVAR && rt==T_INTVAR)
		{
			r=new expression(new expression(r), O_INT_TO_FLOAT)->evaluate();
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
	if (t==T_STRINGVAR) return expression::stringEval();
	switch (this->getOp())
	{
	case O_INVERT:
		this->op=new operands(t);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "= ~" << l->boxName() << ";\n";
		break;
	case O_NEGATE:
		this->op=new operands(t);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "= -" << l->boxName() << ";\n";
		break;
	case O_NOT:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "= !" << l->boxName() << ";\n";
		break;
	case O_INT_TO_FLOAT: /*Note: this duplicates functionality of variable assignment */
		this->op=new operands(T_FLOATVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "= const_cast<double>(" 
			<< l->boxName() << ");\n";
		/* TODO:  Check for divide by zero error and modulo zero error */
	case O_REMAINDER:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "%" << r->boxName() << ";\n";
		break;
	case O_DIVIDE:
		this->op=new operands(t);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "/" << r->boxName() << ";\n";
		break;
	case O_PLUS:
		this->op=new operands(t);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "+" << r->boxName() << ";\n";
		break;
	case O_MINUS:
		this->op=new operands(t);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "-" << r->boxName() << ";\n";
		break;
	case O_MULTIPLY:
		this->op=new operands(t);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "*" << r->boxName() << ";\n";
		break;
	case O_OR:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "|" << r->boxName() << ";\n";
		break;
	case O_AND:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "&" << r->boxName() << ";\n";
		break;
	case O_GREATER:
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << ">" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS:
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "<" << r->boxName() << ")?-1:0;\n";
		break;
	case O_GREATER_EQUAL:
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << ">=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS_EQUAL:
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "<=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_EQUAL:
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "==" << r->boxName() << ")?-1:0;\n";
		break;
	case O_UNEQUAL:
		this->op=new operands(T_INTVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=(" << l->boxName() << "!=" << r->boxName() << ")?-1:0;\n";
		break;
	default:
		errorLevel=E_INTERNAL;
		exit(1);
		break;
	}
	/* convert expression into single operand */
	delete this->left;
	this->left=NULL;
	if (r)
	{
		delete this->right;
		this->right=NULL;
	}
	this->oper=O_TERM;
	return this->op;
}

operands * expression::stringEval(ostream &scope, operands *l, operands *r)
{
	if (this->getOp()==O_STRING_CONCAT)
	{
		this->op=new operands(T_STRINGVAR);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" l->boxName() << "+" << r->boxName();
	}
	else
	{
		errorLevel=E_INTERNAL;
		exit(1);
	}
	/* convert expression into single operand */
	delete this->left;
	this->left=NULL;
	if (r)
	{
		delete this->right;
		this->right=NULL;
	}
	this->oper=O_TERM;
	return this->op;
}

expression::~expression()
{
	if(this->getOp()==O_TERM)
	{
		delete this->op;
	}
	else
	{
		delete this->left;
		if (this->isBinOp())
		{
			delete this->right;
		}
	}
}

/* variable definitions */
variable::variable(ostream &scope, string &name, enum TYPES t):operands(t)
{
	this->generateBox(scope); /*TODO:  FINISH THIS*/
}

variable *variable::getOrCreateVarName(string &name, enum TYPES t)
{
	if (!scopeGlobal)
	{
		return fn::getOrCreateVar(t, name, false);
	}
	/* TODO: verify if type is compatible */
	shared_ptr<operands>op=operands::getOrCreateGlobal(name);
	shared_ptr<variable>v=new variable();
	v->assignment(new expression(op));
	return v;
}

void variable::assignment(expression *value)
{
	operands *op=value->evaluate();
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
