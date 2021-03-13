#include "yab2cpp.h"

enum COMPILEERRORS errorLevel=E_OK;
unsigned int mode=0;
unsigned int indentLevel=0;
bool scopeGlobal=true;

extern ofstream output_cpp;
extern ofstream funcs_h;
extern ofstream heap_h;
extern ofstream consts_h;
extern ofstream logfile;
extern ofstream varNames;

extern ifstream src;

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

/* base class of all the code structure types */

codeType::codeType(enum CODES t)
{
	this->id= ++nextID;
	nesting.push_back(this);
	this->type=t;
}

codeType *codeType::getCurrent()
{
	return nesting.back;
}

void codeType::close()
{
	nesting.pop_back();
}

/* label definitions and helper routines */

label *label::find(string &s)
{
	auto ret=lookup.find(s);
	return(ret==lookup.end()?NULL:ret->second);
}

void label::dumpLabels(ostream &v)
{
	v << "Global Labels\n\n";
	for(auto iter=lookup.begin(); iter!=lookup.end(); ++iter)
	{
		v << "label " << iter->first << " has ID " << iter->second->getID() << "\n" ;
	}
	v << endl;
}

void label::generateJumpTo(ostream &out)
{
	out << "state=" << this->getID() << ";\nbreak;\n";
}

void label::generateOnNSkip(ostream &k, list<label *> &dest)
{
	if (dest->size()<2)
	{
		errorLevel=E_BAD_SYNTAX;
		exit(1);
	}
	auto iter=dest.start(); 
	k << "j" << this->getID() << "[]={" << *iter;
	++iter;
	while(iter!=dest.end())
	{
		k << ", " << *iter;
		++iter;
	}
	k << "}\njs" << this->getID()<< "=" << dest->size() << ";\n";
}

void label::generateOnNTo(ostream &out, expression *e)
{
	operands *o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		out << "if(";
		o->boxName(out);
		out << ">=0 && ";
		o->boxName(out);
		out << "<js" << this->getID() << ")state=j["; 
		o->boxName(out);
		out << "];\nbreak;\n";
	}
	delete e;
}

void label::generateCondJump(ostream &out, expression *e)
{
	operands *o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		out << "if(";
		o->boxName(out);
		out << "!=0)state=" << this->getID() << ";\nbreak;\n";
	}
	delete e;
}

void label::generate(ostream &out)
{
	out << "case " << this->getID() <<":\n";
}

/* conditional definition */

conditional::conditional(ostream &out, expression *e):codeType(T_IF)
{
	this->redo=new label();
	redo->generate(out);
	this->done=new label();
	expression *f=new expression(e,O_NOT);
	this->chain=new label();
	chain->generateCondJump(out, f);
}

void conditional::generateBreak(ostream &out)
{
	done->generateJumpTo(out);
}

void conditional::generateContinue(ostream &out)
{
	redo->generateJumpTo(out);
}

void conditional::alternative(ostream &out, expression *e=NULL)
{
	done->generateJumpTo(out);
	this->chain->generate();
	delete this->chain;
	this->chain=NULL;
	if(e!=NULL)
	{
		this->chain=new label();
		expression *f=new expression(e,O_NOT);
		chain->generateJumpCond(out, f);
	}
}

void conditional::close(ostream &out)
{
	if(this->chain)
	{
		/* elsif ended without else in between */
		errorLevel=E_BAD_SYNTAX;
		exit(1);
	}
	this->done->generate();
}

conditional::~conditional()
{
	delete this->done;
	delete this->redo;
}

/* Loop definitions */
repeatLoop::repeatLoop(ostream &out):codeType(T_REPEATLOOP)
{
	this->loopStart=new label();
	this->loopEnd=new label();
	loopStart->generate(out;)
}

void repeatLoop::generateBreak(ostream &out)
{
	loopEnd->generateJumpTo(out);
}

void repeatLoop::close(ostream &out, expression *e)
{
	expression *f=new expression(e, O_NOT);
	loopStart->generateCondJump(out, f);
	loopEnd->generate(out);
}

repeatLoop::~repeatLoop()
{
	delete loopStart;
	delete loopEnd;
}

doLoop::doLoop(ostream &out):codeType(T_DOLOOP)
{
	this->loopStart=new label();
	this->loopEnd=new label();
	loopStart->generate(out;)
}

void doLoop::generateBreak(ostream &out)
{
	loopEnd->generateJumpTo(out);
}

void doLoop::close(ostream &out)
{
	this->loopStart->generateJumpTo(out);
	this->loopEnd->generate(out);
}

doLoop::~doLoop()
{	delete loopStart;
	delete loopEnd;
}

whileLoop::whileLoop(ostream &out, expression *e):codeType(T_WHILELOOP)
{
	loopContinue=new label();
	loopStart=new label();
	loopEnd=new label();
	cond=e;
	loopStart->generateJumpTo(out);
	loopContinue->generate(out);
}

void whileLoop::generateBreak(ostream &out)
{
	loopEnd->generateJumpTo(out);
}

void whileLoop::close(ostream &out)
{
	loopStart->generate(out);
	loopContinue->generateJumpCond(out, cond);
	loopEnd->generate(out);
}

whileLoop::~whileLoop()
{
	delete loopStart;
	delete loopContinue;
	delete loopEnd;
}

/* TODO: make the stopper into a full range check */
forLoop::forLoop(ostream &out, ostream &k, variable *v, expression *start, expression *stop, expression *stepVal=NULL):codeType(T_FORLOOP)
{
	expression *stopper=new expression(new expression (v), O_UNEQUAL, stop);
	v->assignment(out, start);
	infrastructure=new whileLoop(out, stopper);
	if (stepVal)
	{
		step=stepVal;
	}
	else
	{
		step=new expression(operands::createConst(k, "1", T_INT));
	}
}

void forLoop::generateBreak(ostream &out)
{
	infrastructure->generateBreak(out);
}

void forLoop::close(ostream &out)
{
	expression *stepper=new expression(new expression(v), O_PLUS, step);
	v->assignment(out, stepper)
	infrastructure->close(ostream &out);
}

/* function definitions */

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
