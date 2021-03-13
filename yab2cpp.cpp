/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

enum COMPILE_ERRORS errorLevel=E_OK;
unsigned int mode=0;
unsigned int indentLevel=0;
bool scopeGlobal=true;

ifstream src;
ofstream output_cpp;
ofstream funcs_h;
ofstream heap_h;
ofstream consts_h;
ofstream logfile;
ofstream varNames;

/* private prototypes */
void helpText(string &);
void shutDown();
void logger(string &);

/* process command line parameters */
int main(int argc, char *argv[])
{
	atexit(shutDown);
	switch (argc)
	{
		case 1:
			mode=COMPILE;
			cout << "\nCompile initiated." << endl;
			compile();
			break;
		case 2:
			if (argv[1][0]=='-')
			{
				switch (argv[1][1])
				{
					case 'd':
						cout << "\nIdentifier dump initiated." << endl;
						mode=DUMP;
						compile();
						break;
					case 'v':
						cout << "\n" << argv[0] << " version " 
							<< VER_MAJOR << "." << VER_MINOR << "." << VER_RELEASE << endl;
						break;
					case 'V':
						cout << "\nVerbose compile initiated." << endl;
						mode=DUMP|COMPILE;
						compile();
						break;
					case 'D':
						cout << "\nCompiler debug and dump mode initiated." << endl;
						mode=DUMP|DEBUG;
						compile();
						break;
					case 'G':
						cout << "\nDebug, dump and compile initiated." << endl;
						mode=DUMP|DEBUG|COMPILE;
						compile();
						break;
					default:
						helpText(argv[0]);
						break;
				}
			}
			break;
		default:
			helpText(argv[0]);
			break;
	}
	return 0;
}

/* print the help text to stdout */
void helpText(string &commandname)
{
	cout << commandname << "[-d|D|V|v|G] < filename.mb\n" <<
		"Compiles filename.mb by default unless a flag is specified.\n" <<
		"\n The optional flags are as follows:\n" <<
		"-d is a dump of build to the parse.log file.\n" <<
		"-D is a dump of identifiers and logged build.\n" <<
		"-V is for a verbose build where the compiler logs and compiles.\n" <<
		"-v prints the version and exits.\n\n" <<
		"-G activates dump, debug and compile all at once.\n" << endl;
}

/* open files and initialize them*/
void setUp()
{
	if (mode & COMPILE)
	{
		/* compile mode */
		output_cpp=new ofstream("build/output.cpp");
		funcs_h=new ofstream ("functions.h");
		consts_h=new ofstream("consts.h");
		heap_h=new ofstream("heap.h");
		output_cpp << "#include <runtime.h>\n#include \"consts.h\"\n"
			<< "#include \"heap.h\"\n#include \"functions.h\"\n"
			<< "unsigned int state=start;\nint run(){\nwhile (state>=start){\n"
			<< "switch(state){\ncase start:" << endl;
		if (mode & DEBUG)
		{
			varNames=new ofstream("varnames.txt");
		}
	}
	if (mode & DUMP)
	{
		/* dump identifier mode */
		logfile=fopen("parse.log","w");
		logger("Setup complete.");
	}
}

/* write a note in the logfile */
void logger(string &contents)
{
	unsigned int count;
	if (mode & DEBUG)
	{
		count=indentLevel;
		while (count > 0)
		{
			logfile << '\t';
			--count;
		}
		logfile << contents << endl;
	}
}

/* shutdown the compiler and exit */
void shutDown()
{
	if  (errorLevel != E_OK) cerr << "\nERROR: " << COMPILEERRORNAMES[errorLevel] << "\n\n" << endl;
	if (fn::isCallStackEmpty())
	{
		logger("Stack was empty");
	}
	else
	{
		logger("Dumping stack.");
		if (mode & DUMP && logfile != NULL)
		{
			fn::dumpCallStack(logfile);
		}
	}
	operands::dumpVars();
	label::dumpLabels();
	output_cpp << "}\n}return state;\n}"<< endl;
	}
}

/* open files and compile */
void compile()
{
	setUp();
		
	/* parse */
	ctx = mb_create(NULL);
	while(mb_parse(ctx, NULL)){logger("done");}
	mb_destroy(ctx);

	shutDown();
}

/* methods for operands */
static void operands::dumpVars(ostream &out)
{
	out << "Global Variables\n";
	for(auto iter=globals.begin(); iter!=globals.end(); ++iter)
	{
		out << "variable " << iter->first << " has ID " << iter->second << "\n";
	}
	out << endl;
}

unsigned int operands::getOrCreateStr(ostream &k, string &s)
{
	auto iter=constStr.find(s);
	if (iter!=constStr.end()) return iter->second;
	++nextID;
	k << "const string k" << nextID << "=\"" << s << "\";\n";
	constStr[s]=nextID;
	return nextID;
}

unsigned int operands::createConst(ostream &k, string &s, enum TYPES t)
{
	operands *me=new operands(t);
	if (t==T_INT)
	{
		k << "const int k";
	}
	else
	{
		if (t==T_FLOAT)
		{
			k << "const double k";
		}
		else
		{
			errorLevel=E_TYPE_MISMATCH;
			exit(1);
		}
	}
	k << me->getID() << "=" << s << ";\n";
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
	return T_NONE;
}

enum TYPES operands::coerceTypes()
{
	if this->isBinOp()
	{
		if (l->getSimpleVarType()==T_INTVAR && r->getSimpleVarType()==T_FLOATVAR)
		{
			/* promote l to float */
			t=T_FLOATVAR;
			break;
		}
		if (l->getSimpleVarType()==T_FLOAT && r->getSimpleVarType()==T_INT)
		{
			/* promote r to float */
			t=T_FLOATVAR;
			break;
		}
		if (l->getSimpleVarType()==r->getSimpleVarType())
		{
			break;
		}
		errorLevel=E_TYPE_MISMATCH;
		exit(1);
	}
	else
	{
		if (t==T_NONE)
		{
			errorLevel=E_TYPE_MISMATCH;
		}
	}

}

/* operands used by expression parser and variables */

operands::operands(enum TYPES t)
{
	this->id = ++nextID;
	this->type=t;
}

void operands::generateBox(ostream &out)
{
	switch (this->getSimpleVarType())
	{
	case T_INTVAR:
		out << "int v";
		break;
	case T_FLOATVAR:
		out << "double v";
		break;
	case T_STRINGVAR:
		out << "string v";
		break;
	default:
		errorLevel=E_TYPE_MISMATCH;
		exit(1);
		break;
	}
	out << this->getID() << ";\n";
}

operands *operands::getOrCreateGlobal(ostream &heap, string &s, enum TYPES t)
{
	operands op*=operands::globals->find(s);
	if (op==globals.end())
	{
		op=new variable(heap, s, t);
	}
	return op;
}

void operands::boxName(ostream &out)
{
	switch (this->getType())
	{
	case T_STRINGVAR:
	case T_INTVAR:
	case T_FLOATVAR:
		break;
	
	default:
		errorLevel=E_INTERNAL;
		exit(1);
		break;
	}
	out << "v" << this->getID();
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
			return false;
			break;
	}
	return true;
}

operands *expression::evaluate(ostream &scope, ostream &output_cpp)
{
	operands *l, *r;
	if (this->getOp()==O_TERM) return op;
	l=this->getLeft()->evaluate();
	enum TYPES t=this->coerceTypes();
	l->getSimpleVarType();
	r=(this->isBinOp()?this->getRight()->evaluate():NULL);
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
		this->op=new operands(t);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "= !" << l->boxName() << ";\n";
		break;
		/* TODO:  Check for divide by zero error and modulo zero error */
	case O_REMAINDER:
		this->op=new operands(t);
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
		this->op=new operands(t);
		this->op->generateBox(scope);
		output_cpp << this->op->boxName() << "=" << l->boxName() << "|" << r->boxName() << ";\n";
		break;
	case O_AND:
		this->op=new operands(t);
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
	delete this->left;
	this->left=NULL;
	if (this->isBinOp())
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
	this->generateBox(scope);
}

variable *variable::getOrCreateVarName(ostream &func, ostream &heap, string &name, enum TYPES t)
{
	variable *v;
	if (!scopeGlobal)
	{
		fn *currentFunc=fn::getCurrentSub();
		v=fn::getOrCreateVar(func, heap, t, name, false);
		return v;
	}
	return reinterpret_cast<variable *>operands::getOrCreateGlobal(heap, name);
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
				/* TODO: convert int to float */
			}
			else
			{
				if (t!=T_FLOATVAR)
				{
					errorLevel=E_TYPE_MISMATCH;
					exit(1);
				}
			}
			break;
		default:
			if (t!=this->getType())
			{
				errorLevel=E_TYPE_MISMATCH;
				exit(1);
			}
			break;
	}
	this->boxName(output_cpp);
	output_cpp << "=";
	op->boxName(output_cpp);
	output_cpp << ";\n";
}

/* function definitions */
fn *fn::getCurrentSub()
{
	return callStack.back;
}

void fn::generateOnNSub(ostream &out, expression *e)
{
	this->ret=new label();
	fn::callStack.push_back(this);
	label::generateOnNTo(out, e);
	out << "case " << ret->getID() << ":\n";
}

void fn::generateGosub(ostream &out, shared_ptr<label> sub)
{
	this->ret=new label();
	fn::callStack.push_back(this);
	out << "state=" << sub->getID() << ";\nbreak;\n";
	out << "case " << ret->getID() << ":\n";
}
