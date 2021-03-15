#ifndef TRANSPILER_H
#define TRANSPILER_H
/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include <string>
#include <list>
#include <unordered_map>
#include <memory>
#include <iostream>

using namespace std;

#define VER_MAJOR 0
#define VER_MINOR 0
#define VER_RELEASE 1

extern ofstream output_cpp;
extern ofstream funcs_h;
extern ofstream heap_h;
extern ofstream consts_h;
extern ofstream logfile;
extern ofstream varNames;

/*
** list of all compiler errors 
**
** Note:  There must be a corresponding error message
** to each entry in the COMPILE_ERROR_NAMES constant array.
*/
enum COMPILE_ERRORS {
	E_OK=0,
	E_BAD_SYNTAX,
	E_TYPE_MISMATCH,
	E_BAD_ALLOC,
	E_STACK_UNDERFLOW,
	E_INTERNAL,
	E_DUPLICATE_SYMBOL,
	E_END_FUNCTION,
	E_GOSUB_CANNOT_RETURN_VALUE,
	E_SUBROUTINE_NOT_FOUND,
	E_TOO_MANY_PARAMETERS,
	E_UNASSIGNABLE_TYPE
};

extern enum COMPILE_ERRORS errorLevel;
extern unsigned int mode;
extern unsigned int indentLevel;
extern bool scopeGlobal;

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

/* flags used internally by the compiler
	(must be powers of 2) */
#define COMPILE 1
#define DUMP 2
#define DEBUG 4

/* list of all variable and constant types */
enum TYPES
{
	T_UNKNOWN=0,
	T_NONE,
	T_STRING,
	T_INT,
	T_FLOAT,
	T_STRINGVAR,
	T_INTVAR,
	T_FLOATVAR,
	T_INTCALL_ARRAY,
	T_FLOATCALL_ARRAY,
	T_STRINGCALL_ARRAY,
	T_VOIDCALL
}
/* list of all kinds of other code structures */
enum CODES
{
	T_PRINT=0,
	T_PRINTSEGMENT,
	T_WHILELOOP,
	T_FORLOOP,
	T_REPEATLOOP,
	T_DOLOOP,
	T_IF,
	T_PROCEDURE,
	T_FUNCTION,
	T_GOSUB,
	T_ASSIGNMENT,
	T_LABEL,
	T_PARAMLIST,
	T_DATAITEM,
	T_STRINGFUNC,
	T_FLOATFUNC,
	T_INTFUNC,
	T_VOIDFUNC,
	T_UNKNOWNFUNC
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

typedef union
{
	double d;
	int i;
	string *s;
}boxTypes;

/* subtypes of the T_PRINTSEPARATOR type */
enum SEPARATORS
{
	S_COMMA,
	S_SEMICOLON,
	S_LINEFEED
};

enum OPERATORS
{
	O_PLUS,
	O_MINUS,
	O_MULTIPLY,
	O_DIVIDE,
	O_REMAINDER,
	O_NEGATE,
	O_EXPONENT,
	O_GREATER,
	O_LESS,
	O_EQUAL,
	O_GREATER_EQUAL,
	O_LESS_EQUAL,
	O_UNEQUAL,
	O_NOT,
	O_INVERT,
	O_OR,
	O_AND,
	O_STRING_CONCAT,
	O_INT_TO_FLOAT,
	O_TERM
};

/* global prototype */
void error(enum COMPILE_ERRORS err);

/* internal states used by the parser */
class operands
{
	enum TYPES type;
	unsigned int id;
	static unsigned int nextID;
	static unordered_map<string, operands *> globals;
	static unordered_map<string, unsigned int> strConst;
public:
	enum TYPES getType() const {return type;}
	unsigned int getID() const {return id;}

	static operands *findGlobal(string &s);
	static void dumpVars(ostream &out);
	static unsigned int getOrCreateStr(string &s);
	static operands *createConst(string &s, enum TYPES t);
	static operands *getOrCreateGlobal(string &s, enum TYPES t);

	enum TYPES getSimpleVarType();
	void generateBox(ostream &scope);
	virtual string &boxName();
	enum TYPES coerceTypes();

	explicit operands(enum TYPES t);
	virtual ~operands()
	{}
};

/* expression can be terminal or non-terminal */
class expression
{
	operands *op;
	expression *left;
	expression *right;
	enum OPERATORS oper;
public:
	enum OPERATORS getOp() const {return oper;}
	expression *getLeft() const {return left;}
	expression *getRight() const {return right;}

	bool isBinOp();
	operands *evaluate();
	operands *stringEval();

	/* r is NULL for unary operators */
	expression(expression *l, enum OPERATORS o, expression *r=NULL)
	{
		this->left=l;
		this->right=r;
		this->oper=o;
	}
	expression(operands x)
	{
		op=x;
		oper=O_TERM;
	}
	/*TODO: Recycle temporary variables when not in debug mode*/
	virtual ~expression();
};

/* parent class of all code types */
class codeType
{
	enum CODES type;
	static list<codeType *> nesting;
public:
	enum CODES getType() const {return this->type;}

	static codeType *getCurrent();

	virtual void close();
	virtual void generateBreak()=0;

	explicit codeType(enum CODES t);
	virtual ~codeType()
	{}
};

class label
{
	unsigned int id;
	static unsigned int nextID;
	static unordered_map<string, label *> lookup;
public:
	static dumpLabels(ostream &v);

	unsigned int getID() const {return id;}

	void generateJumpTo();
	void generateOnNSkip(list<shared_ptr<label> >dest);
	void generateOnNTo(expression *e);
	void generateCondJump(expression *e);
	void generate();
	
	static label *find(string &s);

	label(){this->id = ++nextID;}
	label(string &s)
	{
		label();
		lookup[s]=this;
	}

	virtual ~label()
	{}
};

/* if statement */
class conditional:public codeType
{
	label *redo; /* for continue command */
	label *done; /* for break or after "then" condition */
	label *chain; /* For elsif command */
public:
	void generateContinue();
	virtual void generateBreak() override;
	void alternative(expression *e=NULL); /* enable else or elsif condition */
	virtual void close() override; /* end if */

	explicit conditional(expression *e);
	virtual ~conditional();
};

/* looping constructs */
class repeatLoop:public codeType
{
	label *loopStart;
	label *loopEnd;
public:
	virtual void generateBreak() override;
	virtual void close(expression *e) override;

	explicit repeatLoop();
	virtual ~repeatLoop();
};

class doLoop:public codeType
{
	label *loopStart;
	label *loopEnd;
public:
	virtual void generateBreak() override;
	virtual void close() override;

	explicit doLoop();
	virtual ~doLoop();
};

class whileLoop:public codeType
{
	label *loopContinue;
	label *loopStart;
	label *loopEnd;
	expression *cond;
public:
	virtual void generateBreak() override;
	virtual void close() override;

	explicit whileLoop(expression *e);
	virtual ~whileLoop();
};

class variable:public operands
{
	ostream &myScope;
public:
	static shared_ptr<variable>getOrCreateVarName(string &name, enum TYPES t);

	void assignment(expression *value);
	explicit variable(ostream &scope, string &name, enum TYPES t);
	virtual variable()
	{}
}

class arrayType:public variable
{
	list<unsigned int> dimensions;
public:
	virtual string &boxName(list<unsigned int>indexes) override;

	explicit arrayType(string &name, enum TYPES t, list<unsigned int>dim);/*:variable(scope, name, t);*/
	virtual ~arrayType()
	{}
};

class forLoop:public codeType
{
	variable *var;
	variable *startTemp;
	variable *stopTemp;
	whileLoop *infrastructure;
	expression *step;
public:
	virtual void generateBreak();
	virtual void close();

	explicit forLoop(variable *v, expression *start, expression *stop, expression *stepVal=NULL);
	virtual ~forLoop();
};

class fn:codeType
{
	static unordered_map<string, shared_ptr<variable> >locals;
	static unordered_map<string, shared_ptr<variable> >statics;
	static unordered_map<string, shared_ptr<fn> >functions;
	static list<shared_ptr<fn> > callStack;
	static unsigned int nextID;
	list<shared_ptr<variable> >params;
	unsigned int id;
	enum TYPES kind;
	shared_ptr<label>startAddr;
	shared_ptr<label>ret;
	/* private constructor used by generateGosub and generateOnNSub*/
	fn(label *gosub);
public:
	static variable *getOrCreateVar(enum TYPES t, string &s, bool stat);
	static void dumpCallStack();
	static shared_ptr<fn>getCurrentSub();
	static shared_ptr<fn>getSub(string &name);
	static void generateGosub(shared_ptr<label> sub);
	/* must be called after label::generateOnNSkip */
	static void generateOnNSub(expression *e);

	unsigned int getID() const {return this->id;}
	int getNumParams() const {return this->params.size;}
	void addParameter(shared_ptr<variable>);

	operands *generateCall(string &name, list<shared_ptr<operands> >&paramList);
	operands *generateReturn(expression *expr);
	void generateReturn();
	virtual void generateBreak();
	virtual void close();

	fn(string &name, enum CODES t);
	virtual ~fn()
	{}
};

/* The next two structures are used to implement the PRINT statement. */
class printSegments
{
	expression *cargo;
	enum SEPARATORS kind;
public:
	printSegments(expression *e, enum SEPARATORS k)
	{
		this->cargo=e;
		this->kind=k;
	}
	printSegments(expression *e) {printSegments(e, S_LINEFEED);}
	printSegments() {printSegments(NULL);}
	virtual ~printSegments()
	{}
};

struct printStatement
{
	list<printSegments *> segments;
};

#endif
