/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

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
	"value cannot be assigned"
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
void helpText(string);
void setup();
void compile();
void shutDown();

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
		output_cpp.open("output/output.cpp");
		funcs_h.open("output/functions.h");
		consts_h.open("output/consts.h");
		heap_h.open("output/heap.h");
		output_cpp << "#include <runtime.h>\n#include \"consts.h\"\n"
			<< "#include \"heap.h\"\n#include \"functions.h\"\n"
			<< "unsigned int state=start;\nint run(){\nwhile (state>=start){\n"
			<< "switch(state){\ncase start:" << endl;
		if (mode & DEBUG)
		{
			varNames.open("varnames.txt");
		}
	}
	if (mode & DUMP)
	{
		/* dump identifier mode */
		logfile.open("parse.log");
		logger("Setup complete.");
	}
}

void error(enum COMPILE_ERRORS e)
{
	errorLevel=e;
	exit(1);
}

/* write a note in the logfile */
void logger(string s)
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
		logfile << s << endl;
	}
}

/* shutdown the compiler and exit */
void shutDown()
{
	if  (errorLevel != E_OK) cerr << "\nERROR: " << COMPILE_ERROR_NAMES[errorLevel] << "\n\n" << endl;
	if (fn::isCallStackEmpty())
	{
		logger("Stack was empty");
	}
	else
	{
		logger("Dumping stack.");
		if (mode & DUMP && (logfile))
		{
			fn::dumpCallStack();
		}
	}
	operands::dumpVars();
	label::dumpLabels();
	output_cpp << "}\n}return state;\n}"<< endl;
}

/* open files and compile */
void compile()
{
	setUp();



	shutDown();
}

