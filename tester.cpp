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
	"undimensioned array or undeclared function"
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
	if (DEBUG)
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
		logfile << s << "\n";
	}
}

/* shutdown the compiler and exit */
void shutDown()
{
	if  (errorLevel != E_OK) cerr << "\nERROR: " 
		<< COMPILE_ERROR_NAMES[errorLevel] << "\n\n" << endl;
	logger("Purging tempVar queues");
	tempVar::eraseQueues();
	logger("Dumping stack.");
	if (DUMP && (logfile)) fn::dumpCallStack();
	if (DUMP)
	{
		varNames << "Global Variables\n";
		for(auto iter=globals.begin(); iter!=globals.end(); ++iter)
		{
			varNames << "variable " << iter->first 
				<< " has ID " << iter->second->getID() << "\n";
		}
		varNames << endl;
	label::dumpLabels();
	}
	if (COMPILE) 
	{
		output_cpp << "default:\nstate=UNDEFINED_STATE_ERROR;\n"
			<< "break;\n}\n}\nreturn state;\n}"<< endl;
		funcs_h.flush();
		consts_h.flush();
		heap_h.flush();
	}
	globals.clear();
	locals.clear();
	statics.clear();
}

variableType *v;
printSegment *print;
void testInt()
{
	string name=string("v");
	v=variableType::getOrCreateVar(name, T_INTVAR);
	v->assignment(new expression(new constOp("2", T_INT)));
	print=new printSegment(new expression(v));
	print->generate();
	label::generateEnd();
}

/* open files and compile */
void compile()
{
	setUp();

	testInt();

	shutDown();
}
