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
unsigned int operands::nextID;
unordered_map<string, unsigned int> constOp::strConst;

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
		default:
			break;
	}
	error(E_INTERNAL);
}

/* methods for operands */
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
		default:
		break;
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
					processConst(s);
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
			l=shared_ptr<operands>((new expression(shared_ptr<expression>(
				new expression(l)), O_INT_TO_FLOAT))->evaluate());
			lt=T_FLOATVAR;
		}
		if (lt==T_FLOATVAR && rt==T_INTVAR)
		{
			r=shared_ptr<operands>((new expression(shared_ptr<expression>(
				new expression(r)), O_INT_TO_FLOAT))->evaluate());
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
	switch (this->getOp())
	{
	case O_STRING_CONCAT:
		if (t!=T_STRINGVAR) error(E_BAD_SYNTAX);
		this->op=shared_ptr<operands>(new operands(T_STRINGVAR));
		this->op->generateBox(scopeGlobal?S_GLOBAL:S_LOCAL);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "+" << r->boxName();
		break;
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
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "%" << r->boxName() << ";\n";
		break;
	case O_DIVIDE:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "/" << r->boxName() << ";\n";
		break;
	case O_PLUS:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "+" << r->boxName() << ";\n";
		break;
	case O_MINUS:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "-" << r->boxName() << ";\n";
		break;
	case O_MULTIPLY:
		this->op=shared_ptr<operands>(new operands(t));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "*" << r->boxName() << ";\n";
		break;
	case O_OR:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "|" << r->boxName() << ";\n";
		break;
	case O_AND:
		if (t!=T_INTVAR) error(E_TYPE_MISMATCH);
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=" << l->boxName()
			<< "&" << r->boxName() << ";\n";
		break;
	case O_GREATER:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< ">" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< "<" << r->boxName() << ")?-1:0;\n";
		break;
	case O_GREATER_EQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< ">=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_LESS_EQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< "<=" << r->boxName() << ")?-1:0;\n";
		break;
	case O_EQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< "==" << r->boxName() << ")?-1:0;\n";
		break;
	case O_UNEQUAL:
		this->op=shared_ptr<operands>(new operands(T_INTVAR));
		this->op->generateBox(scopeVar);
		output_cpp << this->op->boxName() << "=(" << l->boxName()
			<< "!=" << r->boxName() << ")?-1:0;\n";
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

/* variable definitions */
variable::variable(enum SCOPES s, string &name, enum TYPES t):operands(t)
{
	this->myScope=s;
	switch (s)
	{
		case S_LOCAL:
			if(locals.find(name)!=locals.end() ||
				statics.find(name)!=statics.end() ) error(E_DUPLICATE_SYMBOL);
			locals[name]=shared_ptr<variable>(this);
			break;		
		case S_GLOBAL:
			if(globals.find(name)!=globals.end()) error(E_DUPLICATE_SYMBOL);
			globals[name]=shared_ptr<variable>(this);
			break;
		case S_STATIC:
			if(locals.find(name)!=locals.end() ||
				statics.find(name)!=statics.end() ) error(E_DUPLICATE_SYMBOL);
			statics[name]=shared_ptr<variable>(this);
			break;
		default:
			error(E_INTERNAL);
	}
}

shared_ptr<variable> variable::getOrCreateVar(string &name, enum TYPES t)
{
	if (!scopeGlobal)
	{
		auto i=locals.find(name);
		if(i!=locals.end())return i->second;
		i=statics.find(name);
		if(i!=statics.end())return i->second;
	}
	if (globals.find(name)!=globals.end())return globals[name];
	shared_ptr<variable>v=shared_ptr<variable>(new variable(
		scopeGlobal?S_GLOBAL:S_LOCAL, name, t));
	v->generateBox(scopeGlobal?S_GLOBAL:S_LOCAL);
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

string arrayType::boxName(list<shared_ptr<operands> >indexes)
{
	ostringstream out;
	string buf;
	auto i=indexes.begin();
	out << 'v' << this->getID();
	while (i!=indexes.end())
	{
		out << '[' << (*i)->boxName() << ']';
		++i;
	}
	out.str(buf);
	out.clear();
	return buf;
}

string arrayType::generateBox(enum SCOPES s)
{
	ostringstream out;
	string buf;
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
	out.str(buf);
	out.clear();
	return buf;
}

arrayType::arrayType(string &name, enum TYPES t, list<unsigned int>dim):
	variable(S_GLOBAL, name, t)
{
	this->dimensions=dim;
}

void arrayType::assignment(list<shared_ptr<expression> >indexes,
	shared_ptr<expression>value)
{
	list<shared_ptr<operands>>x;
	shared_ptr<operands>op=value->evaluate();
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
}
