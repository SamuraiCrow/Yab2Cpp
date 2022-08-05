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
#include <vector>
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
class expression;
class fn;
extern ofstream output_cpp;
extern ofstream funcs_h;
extern ofstream heap_h;
extern ofstream consts_h;
extern ofstream logfile;
extern ofstream varNames;
extern unordered_map<string, unique_ptr<variableType> >globals;
extern unordered_map<string, unique_ptr<variableType> >locals;
extern unordered_map<string, unique_ptr<variableType> >statics;
extern const string CODETYPES[];
extern const string TYPENAMES[];

/*
** list of all compiler errors 
**
** Note:  There must be a corresponding error message
** to each entry in the COMPILE_ERROR_NAMES constant array.
*/
enum COMPILE_ERRORS:unsigned int
{
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
	E_UNDIMENSIONED_ARRAY,
	E_RETURN_CODE_OMITTED,
	E_WRONG_NUMBER_OF_DIMENSIONS
};

extern enum COMPILE_ERRORS errorLevel;
extern unsigned int indentLevel;
/*TODO: Replace scopeGlobal with currentFunc==nullptr*/
extern bool scopeGlobal;
extern fn *currentFunc;
extern unsigned int callEnumerator;

/* flags used internally by the compiler */
extern bool COMPILE;
extern bool DUMP;
extern bool DEBUG;
extern bool TRACE;

/* list of all variableType and constant types */
enum TYPES:unsigned int
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
enum CODES:unsigned int
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
	T_UNKNOWNFUNC
};

/* subtypes of the T_PRINTSEPARATOR type */
enum SEPARATORS:unsigned int
{
	S_COMMA,
	S_SEMICOLON,
	S_LINEFEED
};

enum SCOPES:unsigned int
{
	S_UNKNOWN,
	S_LOCAL,
	S_STATIC,
	S_GLOBAL,
	S_PARAMETER
};

enum OPERATORS:unsigned int
{
	O_NOP,
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
	
class operands
{
	enum TYPES type;
	unsigned int id;
	static unsigned int nextID;
protected:
	/* constructor for parameter passing */
	explicit operands(unsigned int id, enum TYPES t);
	explicit operands(enum TYPES t);
	virtual ~operands()
	{}
public:
	enum TYPES getType() const {return type;}
	unsigned int getID() const {return id;}

	enum TYPES getSimpleVarType();
	virtual string boxName();
	static enum TYPES getSimpleVarType(enum TYPES t);
	void generateBox(enum SCOPES s);

	void assignment(expression *value, bool paramDef=false);
	static operands *createOp(enum TYPES t);
	virtual void dispose();
};

class tempVar:public operands
{
	static list<tempVar *>intQueue;
	static list<tempVar *>floatQueue;
	static list<tempVar *>stringQueue;

	/* private constructor called by getOrCreateVar */
	explicit tempVar(enum TYPES t);
	/* private destructor called by eraseQueues */
	virtual ~tempVar()
	{}
public:
	/* factory method to recycle existing tempVar */
	static tempVar *getOrCreateVar(enum TYPES t);

	/* purge queues at end of compile */
	static void eraseQueues();
	/* recycle tempVar in a queue */
	virtual void dispose();
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
	void assignment(expression *v);
public:
	virtual string boxName(){return box;}

	constOp(const string &s, enum TYPES t);
	~constOp()
	{}
};

/* expression can be terminal or non-terminal node */
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

	/* r is NULL for unary operators */
	expression(expression *l, enum OPERATORS o, expression *r=nullptr);

	/* Terminal expression node */
	expression(operands *x);
	virtual ~expression()
	{};
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
	static unordered_map<string, unique_ptr<label> > lookup;
public:
	static void dumpLabels();
	static void generateEnd();

	unsigned int getID() const {return id;}

	void generateJumpTo();
	/* pass generateOnNSkip as second paramater
		to generateOnNTo or generateOnNSub */
	static unsigned int generateOnNSkip(list<label *>&dest);
	static void generateOnNTo(expression *e, unsigned int skip);
	void generateCondJump(expression *e);
	void generate();
	
	static label *find(string &s);

	label(){this->id = ++nextID;}
	label(string &s)
	{
		this->id = ++nextID;
		label::lookup[s]=unique_ptr<label>(this);
	}

	virtual ~label()
	{}
};

/* if statement */
class ifStatement:public codeType
{
	/* for continue command */
	label *redo;
	/* for break or after "then" condition */
	label *done;
	/* For elsif command */
	label *chain;
public:
	void generateContinue();
	virtual void generateBreak();
	/* enable else or elsif condition */
	void alternative(expression *e=nullptr);
	/* end if */
	virtual void close();

	explicit ifStatement(expression *e);
	virtual ~ifStatement();
};

/* looping constructs */
class repeatLoop:public codeType
{
	label *loopStart;
	label *loopEnd;
public:
	virtual void generateBreak();
	virtual void close(expression *e);

	explicit repeatLoop();
	virtual ~repeatLoop();
};

class doLoop:public codeType
{
	label *loopStart;
	label *loopEnd;
public:
	virtual void generateBreak();
	virtual void close();

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
	virtual void generateBreak();
	virtual void close();

	explicit whileLoop(expression *e);
	virtual ~whileLoop();
};

class variableType:public operands
{
	enum SCOPES myScope;
	fn *handle;
public:
	static variableType *getOrCreateVar(string &name, enum TYPES t);
	virtual string boxName();
	/* always call generateBox() after new variableType() */
	variableType(enum SCOPES s, string &name, enum TYPES t, fn *fnHandle);
	~variableType()
	{}
};

class arrayType:public variableType
{
	list<unsigned int> dimensions;

	list<operands *> &evaluateIndexes(list<expression *>indexes);
	string sourceName(list<operands *> source);
public:
	void generateBox(enum SCOPES s);
	operands *getElement(list<expression *>indexes);
	virtual string boxName(){error(E_UNDIMENSIONED_ARRAY);}

	void assignment(list<expression *>indexes, expression *value);

	explicit arrayType(string &name, enum TYPES t, list<unsigned int>dim);
		/*:variableType(scope, name, t);*/
	virtual ~arrayType()
	{}
};

class forLoop:public codeType
{
	variableType *var;
	tempVar *startTemp;
	tempVar *stopTemp;
	whileLoop *infrastructure;
	expression *step;
public:
	virtual void generateBreak();
	virtual void close();

	explicit forLoop(variableType *v, expression *start, expression *stop,
		expression *stepVal=nullptr);
	virtual ~forLoop();
};

class fn
{
	static unordered_map<string, unique_ptr<fn> > functions;
	static unsigned int nextID;
	unordered_map<string, variableType *>parameters;
	vector<variableType *>params;
	unsigned int id;
	enum CODES type;
	enum TYPES kind;
	operands *rc; // Return Code
	label *startAddr;
	label *skipDef;
	/* stamdard constructor called by declare */
	fn(enum CODES t, operands *returnCode=nullptr);
public:
	static fn *getSub(string &name);
	static void generateGosub(label * sub);
	/* must be called after label::generateOnNSkip */
	static void generateOnNSub(expression *e, unsigned int skip);

	label *getStart() const {return startAddr;}
	enum CODES getType() const {return this->type;}
	unsigned int getID() const {return this->id;}
	size_t getNumParams() const {return this->params.size();}
	/* can return nullptr if not found */
	variableType *getLocalVar(string &name);
	void addParameter(string &name, enum TYPES t);

	operands *generateCall(string &name, list<operands *>&paramList);
	/* standard return is for call */
	void generateReturn(expression *expr);
	/* parameterless return is for gosub */
	void generateReturn();
	virtual void generateBreak();
	virtual void close();

	static fn *declare(string &name, enum CODES t, operands *returnCode=nullptr);
	static void dumpFunctionIDs();

	/* is called only by unique_ptr in static "functions" hash table */
	virtual ~fn();
};

class printSegment
{
	expression *cargo;
	enum SEPARATORS sep;
public:
	void generate();
	printSegment(expression *e=nullptr, enum SEPARATORS s=S_LINEFEED);
	virtual ~printSegment()
	{}
};

#endif
