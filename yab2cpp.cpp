/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

unordered_map<string, shared_ptr<variable> >globals;
unordered_map<string, shared_ptr<variable> >locals;
unordered_map<string, shared_ptr<variable> >statics;

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
	atexit(shutDown);
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
		output_cpp << "#include <runtime.h>\n#include \"consts.h\"\n"
			<< "#include \"heap.h\"\n#include \"functions.h\"\n"
			<< "unsigned int state=start;\nint run(){\nwhile (state>=start){\n"
			<< "switch(state){\ncase start:" << endl;
		if (DUMP)
		{
			varNames.open("varnames.txt");
		}
	}
	if (DEBUG)
	{
		/* dump identifier mode */
		logfile.open("parse.log");
		logger("Setup complete.");
	}
}

[[noreturn]] void error(enum COMPILE_ERRORS e)
{
	errorLevel=e;
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
	if  (errorLevel != E_OK) cerr << "\nERROR: " << COMPILE_ERROR_NAMES[errorLevel] << "\n\n" << endl;
	logger("Dumping stack.");
	if (DUMP && (logfile))
	{
		fn::dumpCallStack();
	}
	varNames << "Global Variables\n";
	for(auto iter=globals.begin(); iter!=globals.end(); ++iter)
	{
		varNames << "variable " << iter->first << " has ID " << iter->second << "\n";
	}
	varNames << endl;
	label::dumpLabels();
	output_cpp << "}\n}return state;\n}"<< endl;
}

/* open files and compile */
void compile()
{
	setUp();



	shutDown();
}

