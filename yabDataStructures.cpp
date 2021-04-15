/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

/* forward declaration and static initializers */
class fn;
unsigned int operands::nextID=0;
unordered_map<string, unsigned int> constOp::strConst;
list<tempVar *>tempVar::intQueue;
list<tempVar *>tempVar::floatQueue;
list<tempVar *>tempVar::stringQueue;


/* methods for operands */
enum TYPES operands::getSimpleVarType(enum TYPES t)
{
	switch (t)
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
		default:
			break;
	}
	error(E_UNASSIGNABLE_TYPE);
}

enum TYPES operands::getSimpleVarType()
{
	return getSimpleVarType(this->getType());
}

/* operands used by expression parser and variables */
operands::operands(enum TYPES t)
{
	this->id = ++nextID;
	this->type=t;
}

void operands::generateBox(enum SCOPES s)
{
	ostringstream ss;
	switch (this->getType())
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
	switch (s)
	{
		case S_LOCAL:
			funcs_h << ss.str();
			return;
		case S_GLOBAL:
		case S_STATIC:
			heap_h << ss.str();
			return;
		default:
			break;
	}
	error(E_INTERNAL);
}

operands *operands::createOp(enum TYPES t)
{
	if (TRACE)
	{
		indent();
		logfile << "Creating operand of type "
			<< TYPENAMES[t] << endl;
	}
	return tempVar::getOrCreateVar(t);
}

/* only tempVar type needs disposing */
void operands::dispose()
{}

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
		x=s.str();
		s.clear();
		return x;
		break;
	
	default:
		error(E_INTERNAL);
	}
}

tempVar::tempVar(enum TYPES t):operands(t)
{
	generateBox(S_GLOBAL);
}

tempVar *tempVar::getOrCreateVar(enum TYPES t)
{
	if (t!=getSimpleVarType(t))
	{
		error(E_TYPE_MISMATCH);
	}
	switch (t)
	{
	case T_INTVAR:
		if (intQueue.empty())
		{
			return new tempVar(t);
		}
		else
		{
			tempVar *x=intQueue.back();
			intQueue.pop_back();
			return x;
		}
		break;
	case T_FLOATVAR:
		if (floatQueue.empty())
		{
			return new tempVar(t);
		}
		else
		{
			tempVar *x=floatQueue.back();
			floatQueue.pop_back();
			return x;
		}
		break;
	case T_STRINGVAR:
		if (stringQueue.empty())
		{
			return new tempVar(t);
		}
		else
		{
			tempVar *x=stringQueue.back();
			stringQueue.pop_back();
			return x;
		}
	default:
		break;
	}
	/* unreachable code */
	error(E_INTERNAL);
}

void tempVar::dispose()
{
	switch(this->getType())
	{
		case T_STRINGVAR:
			stringQueue.emplace_front(this);
			return;
		case T_FLOATVAR:
			floatQueue.emplace_front(this);
			return;
		case T_INTVAR:
			intQueue.emplace_front(this);
			return;
		default:
			break;
	}
	error(E_INTERNAL);
}

void tempVar::eraseQueues()
{
	tempVar *i;
	while(!intQueue.empty())
	{
		i=intQueue.back();
		if (DUMP)
		{
			logfile << "variable " << i->boxName()
				<< " is a temporary integer\n";
		}
		delete i;
		intQueue.pop_back();
	}
	while(!floatQueue.empty())
	{
		i=floatQueue.back();
		if (DUMP)
		{
			logfile << "variable " << i->boxName()
				<< " is a temporary floating point\n";
		}
		delete i;
		floatQueue.pop_back();
	}
	while(!stringQueue.empty())
	{
		i=stringQueue.back();
		if (DUMP)
		{
			logfile << "variable " << i->boxName()
				<< " is a temporary string\n";
		}
		delete i;
		stringQueue.pop_back();
	}
	if (DUMP) logfile << endl;
}

void constOp::processConst(unsigned int i)
{
	stringstream me;
	me << 'k' << i;
	box=me.str();
}

void constOp::processConst( const string &s)
{
	processConst(getID());
	consts_h << box << "=" << s << ";\n";
}


/* constructor for constOp */
constOp::constOp(const string &s, enum TYPES t):operands(t)
{
	/* make sure string folder is initialized */
	unordered_map<string, shared_ptr<operands> >strConst;
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
				auto i=constOp::strConst.find(s);
				if (i!=constOp::strConst.end())
				{
					processConst((*i).second);
				}
				else
				{
					consts_h << "const string ";
					processConst(getID());
					consts_h << box << "=\"" << s << "\";\n";
					constOp::strConst[s]=getID();
				}
			}
			break;
		default:
			error(E_TYPE_MISMATCH);
			break;
	}
}

/* expression parsing routines */
expression::expression(expression *l, enum OPERATORS o, expression *r)
{
	this->left=l;
	this->right=r;
	this->oper=o;
}

expression::expression(operands *x)
{
	this->op=x;
	this->oper=O_TERM;
}

/* binary vs. unary ops */
bool expression::isBinOp()
{
	switch (this->getOp())
	{
		/* fallthrough for multiselect */
		case O_NEGATE:
		case O_NOT:
		case O_INVERT:
		case O_INT_TO_FLOAT:
			return false;
		default:
		return true;
	}
	/* unreachable code */
	error(E_INTERNAL);
}

operands *expression::evaluate()
{
	if (this->getOp()==O_TERM) return op;
	operands *l;
	operands *r;
	enum TYPES t;
	logger("evaluating left");
	l=this->getLeft()->evaluate();
	logger("left evaluated");
	if (this->isBinOp())
	{
		logger("evaluating right");
		r=this->getRight()->evaluate();
		logger("evaluated right");
		enum TYPES lt=l->getSimpleVarType();
		enum TYPES rt=r->getSimpleVarType();
		if (lt==T_INTVAR && rt==T_FLOATVAR)
		{
			l=(new expression(new expression(l), O_INT_TO_FLOAT))->evaluate();
			lt=T_FLOATVAR;
		}
		if (lt==T_FLOATVAR && rt==T_INTVAR)
		{
			r=(new expression(new expression(r), O_INT_TO_FLOAT))->evaluate();
			rt=T_FLOATVAR;
		}
		if (lt!=rt)error(E_TYPE_MISMATCH);
		t=lt;
	}
	else
	{
		t=l->getSimpleVarType();
		r=nullptr;
	}
	switch (this->getOp())
	{
	case O_STRING_CONCAT:
		if (t!=T_STRINGVAR) error(E_BAD_SYNTAX);
		this->op=operands::createOp(T_STRINGVAR);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "+" << r->boxName();
		break;
	case O_INVERT:
		this->op=operands::createOp(t);
		output_cpp << this->op->boxName() << "= ~" << l->boxName() << ";\n";
		break;
	case O_NEGATE:
		this->op=operands::createOp(t);
		output_cpp << this->op->boxName() << "= -" << l->boxName() << ";\n";
		break;
	case O_NOT:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "= !" << l->boxName() << ";\n";
		break;
	case O_INT_TO_FLOAT: /*Note: this duplicates functionality of variable assignment */
		this->op=operands::createOp(T_FLOATVAR);
		output_cpp << this->op->boxName() << "= const_cast<double>(" 
			<< l->boxName() << ");\n";
		/* TODO:  Check for divide by zero error and modulo zero error */
	case O_REMAINDER:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "%" << r->boxName() << ";\n";
		break;
	case O_DIVIDE:
		this->op=operands::createOp(t);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "/" << r->boxName() << ";\n";
		break;
	case O_PLUS:
		this->op=operands::createOp(t);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "+" << r->boxName() << ";\n";
		break;
	case O_MINUS:
		this->op=operands::createOp(t);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "-" << r->boxName() << ";\n";
		break;
	case O_MULTIPLY:
		this->op=operands::createOp(t);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "*" << r->boxName() << ";\n";
		logger("multiply done");
		break;
	case O_OR:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "|" << r->boxName() << ";\n";
		break;
	case O_AND:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "&" << r->boxName() << ";\n";
		break;
	case O_GREATER:
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< ">" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS:
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< "<" << r->boxName() << ")?-1:0;\n";
		break;
	case O_GREATER_EQUAL:
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< ">=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS_EQUAL:
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< "<=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_EQUAL:
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< "==" << r->boxName() << ")?-1:0;\n";
		break;
	case O_UNEQUAL:
		this->op=operands::createOp(T_INTVAR);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< "!=" << r->boxName() << ")?-1:0;\n";
		break;
	default:
		error(E_INTERNAL);
	}
	/* convert expression into single operand */
	logger("deleting left");
	delete left;
	logger("left deleted");
	if (isBinOp())
	{
		logger("deleting right");
		delete right;
		logger("right deleted");
	}
	this->oper=O_TERM;
	return this->op;
}

expression::~expression()
{
	if(this->getOp()==O_TERM)
	{
		op->dispose();
	}
}

/* variable definitions */
variableType::variableType(enum SCOPES s, string &name, enum TYPES t, fn *fnHandle):operands(t)
{
	this->myScope=s;
	this->handle=fnHandle;
	switch (s)
	{
		case S_LOCAL:
			if(handle->getLocalVar(name)) error(E_DUPLICATE_SYMBOL);
			locals[name]=unique_ptr<variableType>(this);
			break;		
		case S_GLOBAL:
			if(globals.find(name)!=globals.end()) error(E_DUPLICATE_SYMBOL);
			globals[name]=unique_ptr<variableType>(this);
			break;
		case S_STATIC:
			if(handle->getLocalVar(name)) error(E_DUPLICATE_SYMBOL);
			statics[name]=unique_ptr<variableType>(this);
			break;
		case S_PARAMETER:
			if (handle->getLocalVar(name)) error(E_DUPLICATE_SYMBOL);
			/* parameter is added to function list by addParamter */
			break;
		default:
			error(E_INTERNAL);
	}
}

string variableType::boxName()
{
	ostringstream ss;
	if (myScope==S_LOCAL)
	{
		ss << "sub" << this->handle->getID() << "->v" << this->getID();
		return ss.str();
	}
	ss << "v" << this->getID();
	return ss.str();
}

variableType *variableType::getOrCreateVar(string &name, enum TYPES t)
{
	variableType *v;
	if (currentFunc!=nullptr)
	{
		v=currentFunc->getLocalVar(name);
		if (v!=nullptr) return v;
	}
	if (globals.find(name)!=globals.end())return globals[name].get();
	if (scopeGlobal)
	{
		v=new variableType(S_GLOBAL, name, t, nullptr);

	}
	else
	{
		v=new variableType(S_LOCAL, name, t, currentFunc);
	}
	v->generateBox(scopeGlobal?S_GLOBAL:S_LOCAL);
	return v;
}

void variableType::assignment(expression *value)
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
	delete value;
}

string arrayType::boxName(list<operands *>indexes)
{
	ostringstream out;
	auto i=indexes.begin();
	out << 'v' << this->getID();
	while (i!=indexes.end())
	{
		out << '[' << (*i)->boxName() << ']';
		++i;
	}
	return out.str();
}

string arrayType::generateBox(enum SCOPES s)
{
	ostringstream out;
	switch (this->getType())
	{
		case T_STRINGCALL_ARRAY:
			out << "string ";
			break;
		case T_INTCALL_ARRAY:
			out << "int ";
			break;
		case T_FLOATCALL_ARRAY:
			out << "double ";
			break;
		default:
			error(E_INTERNAL);
	}
	out << 'v' << this->getID();
	for (auto i=dimensions.begin();i!=dimensions.end();++i)
	{
		out << '[' << *i << ']';
	}
	out << ";\n";
	return out.str();
}

arrayType::arrayType(string &name, enum TYPES t, list<unsigned int>dim):
	variableType(S_GLOBAL, name, t, 0)
{
	this->dimensions=dim;
}

void arrayType::assignment(list<expression *>indexes, expression *value)
{
	list<operands *>x;
	operands *op=value->evaluate();
	enum TYPES t=op->getSimpleVarType();
	auto i=indexes.begin();
	while(i!=indexes.end())
	{
		x.push_back((*i)->evaluate());
		++i;
	}
	switch (this->getType())
	{
	case T_FLOATCALL_ARRAY:
		if (t==T_INTVAR)
		{
			output_cpp << this->boxName(x)
				<< "=static_cast<double>("
				<< op->boxName() << ");\n";
			return;
		}
		if (t!=T_FLOATVAR) error(E_TYPE_MISMATCH);
		break;
	case T_INTCALL_ARRAY:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		break;
	case T_STRINGCALL_ARRAY:
		if (t!=T_STRINGVAR) error(E_TYPE_MISMATCH);
		break;
	default:
		error(E_INTERNAL);
	}
	output_cpp << this->boxName(x) << '=' << op->boxName() <<";\n";
	op->dispose();
	delete value;
}
