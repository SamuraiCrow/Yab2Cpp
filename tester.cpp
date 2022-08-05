/*
**	Tester.cpp
**
**	Transpiler framework tester
**      by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"
#include <cassert>

unordered_map<string, unique_ptr<variableType> >globals;
unordered_map<string, unique_ptr<variableType> >locals;
unordered_map<string, unique_ptr<variableType> >statics;

/* These correspond to the enum COMPILE_ERRORS. */
const char *COMPILE_ERROR_NAMES[]={
	"no error",
	"incorrect syntax",
	"wrong type",
	"failed allocation",
	"stack underflow",
	"internal compiler error",
	"duplicated label",
	"previous subroutine didn't end",
	"value returned from gosub call",
	"undefined subroutine name",
	"too many parameters in function call",
	"value cannot be assigned",
	"undimensioned array or undeclared function",
	"return code was not specified on function",
	"wrong number of dimensions when accessing array"
};

/* These correspond to the types of enum TYPES. */
const string TYPENAMES[]={
	"unknown",
	"none",
	"string constant",
	"integer constant",
	"floating point constant",
	"string variable",
	"integer variable",
	"floating point variable",
	"string array or function",
	"integer array or function",
	"floating point array or function",
	"string array or function",
	"function"
};

const string CODETYPES[]={
	"print sequence",
	"print segment",
	"while loop",
	"for loop",
	"repeat loop",
	"do loop",
	"if statement",
	"procedure statement",
	"function statement",
	"assignment",
	"label",
	"parameter list or array index",
	"data item",
	"function returning string",
	"function returning floating point",
	"function returning integer",
	"function returning nothing",
	"function"
};

enum COMPILE_ERRORS errorLevel=E_OK;
unsigned int indentLevel=0;
bool scopeGlobal=true;
fn *currentFunc=nullptr;

bool COMPILE=false;
bool DUMP=false;
bool DEBUG=false;
bool TRACE=false;

ifstream src;
ofstream output_cpp;
ofstream funcs_h;
ofstream heap_h;
ofstream consts_h;
ofstream logfile;
ofstream varNames;

/* private prototypes */
void helpText(const string);
void setup();
void compile();
void shutDown();

/* process command line parameters */
int main(int argc, char *argv[])
{
	switch (argc)
	{
		case 1:
			COMPILE=true;
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
						DUMP=true;
						compile();
						break;
					case 'v':
						cout << "\n" << argv[0] << " version " 
							<< VER_MAJOR << "." << VER_MINOR << "." << VER_RELEASE << endl;
						break;
					case 'V':
						cout << "\nVerbose compile initiated." << endl;
						DUMP=true;
						COMPILE=true;
						compile();
						break;
					case 'D':
						cout << "\nCompiler debug and dump mode initiated." << endl;
						DUMP=true;
						DEBUG=true;
						compile();
						break;
					case 'G':
						cout << "\nDebug, dump and compile initiated." << endl;
						DUMP=true;
						DEBUG=true;
						COMPILE=true;
						compile();
						break;
					case 't':
						cout << "\nDebug, dump and trace initiated." << endl;
						DEBUG=true;
						DUMP=true;
						TRACE=true;
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
	cout << "program exiting" <<endl;
	return 0;
}

/* print the help text to stdout */
void helpText(const string commandname)
{
	cout << commandname << "[-d|D|V|v|G|t] < filename.mb\n" <<
		"Compiles filename.mb by default unless a flag is specified.\n" <<
		"\n The optional flags are as follows:\n" <<
		"-d is a dump of build to the parse.log file.\n" <<
		"-D is a dump of identifiers and logged build.\n" <<
		"-V is for a verbose build where the compiler logs and compiles.\n" <<
		"-v prints the version and exits.\n" <<
		"-t activates dump, debug and trace\n" <<
		"-G activates dump, debug and compile all at once.\n" << endl;
}

/* open files and initialize them*/
void setUp()
{
	if (COMPILE)
	{
		/* compile mode */
		output_cpp.open("output/output.cpp");
		funcs_h.open("output/functions.h");
		consts_h.open("output/consts.h");
		heap_h.open("output/heap.h");
		output_cpp << "#include \"../runtime/runtime.h\"\n#include \"consts.h\"\n"
			<< "#include \"heap.h\"\n#include \"functions.h\"\n"
			<< "unsigned int state=START;\nunsigned int run(){\n"
			<< "while (state>=START){\n"
			<< "switch(state){\ncase START:" << endl;
	}
	else
	{
		output_cpp.open("/dev/null");
		funcs_h.open("/dev/null");
		consts_h.open("/dev/null");
		heap_h.open("/dev/null");
	}
	if (DUMP)
	{
		varNames.open("varnames.log");
	}
	else
	{
		varNames.open("/dev/null");
	}
	if (DEBUG || TRACE)
	{
		/* dump identifier mode */
		logfile.open("parse.log");
		logger("Setup complete.");
	}
	else
	{
		logfile.open("/dev/null");
	}
}

[[noreturn]] void error(enum COMPILE_ERRORS e)
{
	errorLevel=e;
	shutDown();
	exit(1);
}

void indent()
{
	unsigned int count=indentLevel;
	while (count > 0)
	{
		logfile << '\t';
		--count;
	}

}

/* write a note in the logfile */
void logger(string s)
{
	if (DEBUG)
	{
		indent();
		logfile << s << endl;
	}
}

/* shutdown the compiler and exit */
void shutDown()
{
	if  (errorLevel != E_OK) cerr << "\nERROR: " 
		<< COMPILE_ERROR_NAMES[errorLevel] << "\n\n" << endl;
	logger("Shutting Down");
	if (DUMP)
	{
		fn::dumpFunctionIDs();
		varNames << "Global Variables\n";
		for(auto iter=globals.begin(); iter!=globals.end(); ++iter)
		{
			varNames << "variable " << iter->first 
				<< " has ID " << iter->second->getID() << "\n";
		}
		varNames << endl;
		label::dumpLabels();
	}
	globals.clear();
	locals.clear();
	statics.clear();
	if (COMPILE) 
	{
		output_cpp << "default:\nstate=UNDEFINED_STATE_ERROR;\n"
			<< "break;\n}\n}\nreturn state;\n}"<< endl;
		funcs_h.flush();
		consts_h.flush();
		heap_h.flush();
	}
	tempVar::eraseQueues();
}

variableType *v;
printSegment *print;
operands *o;
expression *e;
list<operands *>p;

void testInt()
{
	logger("testInt started");
	string name=string("v");
	v=variableType::getOrCreateVar(name, T_INTVAR);
	logger("variable v created");
	o=new constOp("2", T_INT);
	logger("constOp 2 created");
	v->assignment(new expression(o));
	logger("assignment created");
	print=new printSegment(new expression(v));
	print->generate();
	delete print;
	logger("testInt cleared.");
}

void testString()
{
	logger("testString started");
	string name=string("name1");
	v=variableType::getOrCreateVar(name, T_STRINGVAR);
	v->assignment(new expression(new constOp("Hello", T_STRING)));
	print=new printSegment(new expression(v), S_SEMICOLON);
	print->generate();
	delete print;
	print=new printSegment(new expression(new constOp(" world!", T_STRING)));
	print->generate();
	delete print;
	logger("testString cleared");
}

void testFloat()
{
	logger("testFloat started");
	string name=string("floater");
	v=variableType::getOrCreateVar(name, T_FLOATVAR);
	v->assignment(new expression(new constOp("3.14159265", T_FLOAT)));
	print=new printSegment(new expression(v), S_COMMA);
	print->generate();
	delete print;
	print=new printSegment(new expression(new constOp(" is pi", T_STRING)));
	print->generate();
	delete print;
	logger("testFloat cleared");
}

void testFunc()
{
	logger("testFunc started");
	string name=string("square");
	o = operands::createOp(T_FLOATVAR);
	fn *func=fn::declare(name, T_FLOATFUNC, o);
	name=string("radius");
	func->addParameter(name, T_FLOATVAR);
	logger("param added");
	v=variableType::getOrCreateVar(name, T_FLOATVAR);
	e=new expression(new expression(v), O_MULTIPLY, new expression(v));
	logger("expression made");
	func->generateReturn(e);
	logger("return generated");
	func->close();
	logger("function ended");
	o=new constOp("3.0", T_FLOAT);
	p.push_back(o);
	name=string("square");
	o=func->generateCall(name, p);
	logger("call generated");
	e=new expression(o);
	print=new printSegment(e);
	print->generate();
	o->dispose();
	delete print;
	p.clear();
	logger("testFunc cleared");
}

void testIf()
{
	logger("testIf started");
	string name=string("check");
	logger("name declared");
	fn *func2=fn::declare(name, T_UNKNOWNFUNC);
	logger("check fn declared");
	string name2=string("checker");
	func2->addParameter(name2, T_FLOATVAR);
	logger("checker added");
	o=operands::createOp(T_FLOATVAR);
	e=new expression(new expression(o), O_LESS, new expression(new constOp("0.0", T_FLOAT)));
	ifStatement *i=new ifStatement(e);
	print=new printSegment(new expression(new constOp("less", T_STRING)));
	print->generate();
	delete print;
	i->alternative();
	print=new printSegment(new expression(new constOp("greater or equal", T_STRING)));
	print->generate();
	delete print;
	i->close();
	func2->close();
	delete i;
	p.push_back(new constOp("0.0", T_FLOAT));
	func2->generateCall(name, p);
	p.clear();
	p.push_back(new constOp("-0.1", T_FLOAT));
	func2->generateCall(name, p);
	p.clear();
	logger("testIf cleared");
}

void testForLoop()
{
	logger("testForLoop started");
	string name=string("counter");
	logger("creating counter variable");
	v=variableType::getOrCreateVar(name, T_INTVAR);
	logger("initializing forLoop");
	forLoop *f=new forLoop(v,new expression(new constOp("1", T_INT)), 
		new expression(new constOp("10", T_INT)),
		new expression(new constOp("2", T_INT)));
	logger("printing variable");
	print=new printSegment(new expression(v), S_LINEFEED);
	print->generate();
	logger("closing testForLoop");
	f->close();
	delete f;
	logger("disposing of counter variable");
	v->dispose();
	logger("testForLoop cleared");
}

void testOnNCall()
{
	logger("testOnNCall entered");
	string s1 = string("func1");
	fn *f1=fn::declare(s1, T_UNKNOWNFUNC);
	print=new printSegment(new expression(new constOp(string("one"), T_STRING)));
	print->generate();
	delete print;
	f1->close();
	logger("func1 declared");
	string s2=string("func2");
	fn *f2=fn::declare(s2, T_UNKNOWNFUNC);
	print=new printSegment(new expression(new constOp(string("two"), T_STRING)));
	print->generate();
	delete print;
	f2->close();
	logger("func2 declared");
	string s3=string("func3");
	fn *f3=fn::declare(s3, T_UNKNOWNFUNC);
	print=new printSegment(new expression(new constOp(string("three"), T_STRING)));
	print->generate();
	delete print;
	f3->close();
	logger("func3 declared");
	string s=string("countdown");
	variableType *q=variableType::getOrCreateVar(s, T_INTVAR);
	logger("countdown var declared");
	forLoop *f=new forLoop(q,
		new expression(new constOp("3",T_INT)),
		new expression(new constOp("0",T_INT)),
		new expression(new constOp("-1",T_INT))
	);
	logger("forloop declared");
	list<label *> l;
	l.push_back(f1->getStart());
	l.push_back(f2->getStart());
	l.push_back(f3->getStart());
	logger("list allocated");
	unsigned int x = label::generateOnNSkip(l);
	fn::generateOnNSub(new expression(q), x);
	logger("on countdown gosub declared");
	print=new printSegment(new expression(new constOp("fallthrough condition test",T_STRING)));
	print->generate();
	delete print;
	logger("fallthrough condition declared");
	f->close();
	v->dispose();
	l.clear();
	delete f;
	logger("testOnNCall cleared");
}

void testArray() {
	string name=string("rhombic");
	list<unsigned int> dimensions;
	dimensions.push_back(8);
	arrayType *at=new arrayType(name, T_INTCALL_ARRAY, dimensions);
	at->generateBox(S_GLOBAL);
	list<expression *> indexes;
	indexes.push_back(new expression(new constOp("5", T_INT)));
	at->assignment(indexes, new expression(new constOp("4", T_INT)));
	indexes.clear();
	string name2=string("quad");
	variableType *x=variableType::getOrCreateVar(name2, T_INTVAR);
	forLoop *f=new forLoop(x,
		new expression(new constOp("0", T_INT)),
		new expression(new constOp("7", T_INT))
	);
	indexes.push_back(new expression(x));
	print=new printSegment(new expression(x), S_SEMICOLON);
	print->generate();
	delete print;
	print=new printSegment(new expression(new constOp(" has value ", T_STRING)), S_SEMICOLON);
	print->generate();
	delete print;
	print=new printSegment(new expression(at->getElement(indexes)));
	print->generate();
	delete print;
	f->close();
	dimensions.clear();
	delete f;
}

/* open files and compile */
void compile()
{
	setUp();
	testInt();
	testString();
	testFloat();
	testFunc();
	testIf();
	testForLoop();
	testOnNCall();
	testArray();
	logger("generating end");
	label::generateEnd();
	/*check for nesting error */
	if (!scopeGlobal) error(E_END_FUNCTION);
	shutDown();
}
