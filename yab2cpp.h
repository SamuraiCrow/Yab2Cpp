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
#include <fstream>
#include <sstream>

using namespace std;

#define VER_MAJOR 0
#define VER_MINOR 0
#define VER_RELEASE 1

class variableType;
extern ofstream output_cpp;
extern ofstream funcs_h;
extern ofstream heap_h;
extern ofstream consts_h;
extern ofstream logfile;
extern ofstream varNames;
extern unordered_map<string, shared_ptr<variableType> >globals;
extern unordered_map<string, shared_ptr<variableType> >locals;
extern unordered_map<string, shared_ptr<variableType> >statics;
extern const string CODETYPES[];
extern const string TYPENAMES[];

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
	E_UNASSIGNABLE_TYPE,
	E_UNDIMENSIONED_ARRAY
};

extern enum COMPILE_ERRORS errorLevel;
extern unsigned int indentLevel;
extern bool scopeGlobal;

/* flags used internally by the compiler */
extern bool COMPILE;
extern bool DUMP;
extern bool DEBUG;
extern bool TRACE;

/* list of all variableType and constant types */
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
};

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

enum SCOPES
{
	S_UNKNOWN,
	S_LOCAL,
	S_STATIC,
	S_GLOBAL
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
[[noreturn]] void error(enum COMPILE_ERRORS err);
void indent();
void logger(string s);
	
/* internal states used by the parser */
class scope:public ofstream
{
	enum SCOPES myscope;
public:
	ofstream &operator<<(ostream &in);
	enum SCOPES getScope() const {return myscope;}

	scope(enum SCOPES s){myscope=s;}
	~scope()
	{}
};

class operands
{
	enum TYPES type;
	unsigned int id;
	static unsigned int nextID;
	/* private constructor for parameter passing only */
	explicit operands(unsigned int id, enum TYPES t);
public:
	enum TYPES getType() const {return type;}
	unsigned int getID() const {return id;}

	enum TYPES getSimpleVarType();
	void generateBox(enum SCOPES s);
	virtual string boxName();
	enum TYPES coerceTypes();

	explicit operands(enum TYPES t);
	virtual ~operands()
	{}
};

/* constant operands */
class constOp:public operands
{
	/* box is defined once in the constructor */
	string box;
	static unordered_map<string, unsigned int> strConst;

	/* const for id that exists already */
	void processConst(unsigned int i);
	/* const that must be defined still */
	void processConst(const string &s);
public:
	virtual string boxName(){return box;}

	constOp(const string &s, enum TYPES t);
	~constOp()
	{}
};

/* expression can be terminal or non-terminal node */
class expression
{
	shared_ptr<operands>op;
	shared_ptr<expression>left;
	shared_ptr<expression>right;
	enum OPERATORS oper;
public:
	enum OPERATORS getOp() const {return oper;}
	shared_ptr<expression>getLeft() const {return left;}
	shared_ptr<expression>getRight() const {return right;}

	bool isBinOp();
	shared_ptr<operands>evaluate();
	shared_ptr<operands>stringEval(shared_ptr<operands>l,
		shared_ptr<operands>r);

	/* r is NULL for unary operators */
	expression(shared_ptr<expression>l, enum OPERATORS o,
		shared_ptr<expression>r=NULL)
	{
		this->left=l;
		this->right=r;
		this->oper=o;
	}

	/* Terminal expression node */
	expression(shared_ptr<operands>x)
	{
		op=x;
		oper=O_TERM;
	}
	/*TODO: Recycle temporary variableTypes when not in debug mode*/
	virtual ~expression()
	{}
};

/* parent class of all code types */
class codeType
{
	enum CODES type;
public:
	enum CODES getType() const {return this->type;}

	explicit codeType(enum CODES t);
	virtual ~codeType();
};

class label
{
	unsigned int id;
	static unsigned int nextID;
	static unordered_map<string, shared_ptr<label> > lookup;
public:
	static void dumpLabels();

	unsigned int getID() const {return id;}

	void generateJumpTo();
	/* pass generateOnNSkip as second paramater
		to generateOnNTo or generateOnNSub */
	unsigned int generateOnNSkip(list<shared_ptr<label> >&dest);
	static void generateOnNTo(shared_ptr<expression>e, unsigned int skip);
	void generateCondJump(shared_ptr<expression>e);
	void generate();
	
	static shared_ptr<label>find(string &s);

	label(){this->id = ++nextID;}
	label(string &s)
	{
		unordered_map<string, shared_ptr<label> >lookup;
		label();
		label::lookup[s]=shared_ptr<label>(this);
	}

	virtual ~label()
	{}
};

/* if statement */
class ifStatement:public codeType
{
	/* for continue command */
	shared_ptr<label>redo;
	/* for break or after "then" condition */
	shared_ptr<label>done;
	/* For elsif command */
	shared_ptr<label>chain;
public:
	void generateContinue();
	virtual void generateBreak();
	/* enable else or elsif condition */
	void alternative(shared_ptr<expression>e=NULL);
	/* end if */
	virtual void close();

	explicit ifStatement(shared_ptr<expression>e);
	virtual ~ifStatement()
	{}
};

/* looping constructs */
class repeatLoop:public codeType
{
	shared_ptr<label>loopStart;
	shared_ptr<label>loopEnd;
public:
	virtual void generateBreak();
	virtual void close(shared_ptr<expression>e);

	explicit repeatLoop();
	virtual ~repeatLoop()
	{}
};

class doLoop:public codeType
{
	shared_ptr<label>loopStart;
	shared_ptr<label>loopEnd;
public:
	virtual void generateBreak();
	virtual void close();

	explicit doLoop();
	virtual ~doLoop()
	{}
};

class whileLoop:public codeType
{
	shared_ptr<label>loopContinue;
	shared_ptr<label>loopStart;
	shared_ptr<label>loopEnd;
	shared_ptr<expression>cond;
public:
	virtual void generateBreak();
	virtual void close();

	explicit whileLoop(shared_ptr<expression>e);
	virtual ~whileLoop()
	{}
};

class variableType:public operands
{
	enum SCOPES myScope;
public:
	static shared_ptr<variableType>getOrCreateVar(string &name, enum TYPES t);

	void assignment(shared_ptr<expression>value);
	/* always call generateBox() after new variableType() */
	variableType(enum SCOPES s, string &name, enum TYPES t);
	variableType();
	~variableType()
	{}
};

class arrayType:public variableType
{
	list<unsigned int> dimensions;
public:
	string generateBox(enum SCOPES s);
	virtual string boxName(list<shared_ptr<operands> >indexes);
	virtual string boxName(){error(E_UNDIMENSIONED_ARRAY);}

	void assignment(list<shared_ptr<expression> >indexes,
		shared_ptr<expression>value);

	explicit arrayType(string &name, enum TYPES t, list<unsigned int>dim);
		/*:variableType(scope, name, t);*/
	virtual ~arrayType()
	{}
};

class forLoop:public codeType
{
	shared_ptr<variableType>var;
	shared_ptr<variableType>startTemp;
	shared_ptr<variableType>stopTemp;
	shared_ptr<whileLoop>infrastructure;
	shared_ptr<expression>step;
public:
	virtual void generateBreak();
	virtual void close();

	explicit forLoop(shared_ptr<variableType>v, shared_ptr<expression>start,
		shared_ptr<expression>stop, shared_ptr<expression>stepVal=NULL);
	virtual ~forLoop()
	{}
};

class fn:codeType
{
	static unordered_map<string, shared_ptr<fn> > functions;
	static list<shared_ptr<fn> > callStack;
	static unsigned int nextID;
	list<shared_ptr<variableType> >params;
	string funcName;
	unsigned int id;
	enum TYPES kind;
	shared_ptr<operands>rc;
	/* two labels common to all subroutine calls */
	shared_ptr<label>startAddr;
	shared_ptr<label>ret;
	/* private constructor used by generateGosub and generateOnNSub*/
	fn(shared_ptr<label>gosub);
public:
	static void dumpCallStack();
	static shared_ptr<fn>getCurrentSub();
	static shared_ptr<fn>getSub(string &name);
	static void generateGosub(shared_ptr<label> sub);
	/* must be called after label::generateOnNSkip */
	static void generateOnNSub(shared_ptr<expression>e, unsigned int skip);

	unsigned int getID() const {return this->id;}
	int getNumParams() const {return this->params.size();}
	void addParameter(shared_ptr<variableType>);

	shared_ptr<operands>generateCall(string &name,
		list<shared_ptr<operands> >&paramList);
	void generateReturn(shared_ptr<expression>expr);
	void generateReturn();
	virtual void generateBreak();
	virtual void close();

	fn(string &name, enum CODES t, shared_ptr<operands>returnCode=NULL);
	virtual ~fn()
	{}
};

class printSegment
{
	shared_ptr<expression>cargo;
	enum SEPARATORS sep;
public:
	void generate();
	printSegment(shared_ptr<expression>e, enum SEPARATORS s);
	printSegment(shared_ptr<expression>e) {printSegment(e, S_LINEFEED);}
	printSegment() {printSegment(NULL);}
	virtual ~printSegment()
	{}
};

#endif
