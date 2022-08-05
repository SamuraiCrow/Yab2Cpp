/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"
#include "runtime/runtime.h"

/* static initializers */
unordered_map<string, unique_ptr<label> > label::lookup;
unsigned int label::nextID=START;

/* base class of all the code structure types */
codeType::codeType(enum CODES t)
{
	this->type=t;
	if (TRACE)
	{
		indent();
		logfile << "Entering " << CODETYPES[t];
	}
	++indentLevel;
}

codeType::~codeType()
{
	--indentLevel;
	if (TRACE)
	{
		indent();
		logfile << "Leaving " << CODETYPES[this->getType()];
	}
}

/* label definitions and helper routines */
label *label::find(string &s)
{
	unordered_map<string, shared_ptr<label> >lookup;
	auto ret=label::lookup.find(s);
	return(ret==label::lookup.end()?nullptr:ret->second.get());
}

void label::dumpLabels()
{
	varNames << "Global Labels\n" << endl;
	for(auto iter=lookup.begin(); iter!=lookup.end(); ++iter)
	{
		varNames << "label " << iter->first << " has ID "
			<< iter->second->getID() << endl ;
	}
}

void label::generateEnd()
{
	output_cpp << "state=EXIT;\nbreak;\n";
}

void label::generateJumpTo()
{
	output_cpp << "{state=" << this->getID() << ";break;}\n";
}

/* pass this as second parameter to generateOnNTo or generateOnNSub */
unsigned int label::generateOnNSkip(list<label *>&dest)
{
	unsigned int ret=++nextID;
	if (dest.size()<2)error(E_BAD_SYNTAX);
	auto iter=dest.begin();
	consts_h << "const unsigned int j" << ret << "[]={\n" << (*iter)->getID();
	++iter;
	while(iter!=dest.end())
	{
		consts_h << ",\n" << (*iter)->getID();
		++iter;
	}
	consts_h << "\n};\nconst int js" << ret << "=" << dest.size() << ";\n";
	return ret;
}

void label::generateOnNTo(expression *e, unsigned int skip)
{
	// indexed by one instead of zero so we subtract one
	expression *e2=new expression(e, O_MINUS,
		new expression(new constOp("1", T_INT)));
	operands *o=e2->evaluate();
	output_cpp << "if(" << o->boxName() << ">=0 && "
	    << o->boxName() << "<js" << skip 
		<< "){state=j" << skip << "["
	    << o->boxName() << "];break;}\n";
	o->dispose();
	delete e2;
	return;
}

void label::generateCondJump(expression *e)
{
	operands *o=e->evaluate();
	if (o->getSimpleVarType()!=T_INTVAR)error(E_TYPE_MISMATCH);
	output_cpp<< "if(" << o->boxName() 
		<< "!=0){state=" << this->getID() << ";break;}\n";
	o->dispose();
	delete e;
}

void label::generate()
{
	output_cpp<< "case " << this->getID() <<":\n";
}

/* ifStatement definition */

ifStatement::ifStatement(expression *e):codeType(T_IF)
{
	logger("creating ifStatement");
	this->redo=new label();
	redo->generate();
	this->done=new label();
	expression *f=new expression(e,O_NOT);
	this->chain=new label();
	chain->generateCondJump(f);
	logger("ifStatement created");
}

void ifStatement::generateBreak()
{
	done->generateJumpTo();
}

void ifStatement::generateContinue()
{
	redo->generateJumpTo();
}

void ifStatement::alternative(expression *e)
{
	done->generateJumpTo();
	this->chain->generate();
	delete this->chain;
	this->chain=nullptr;
	if(e!=nullptr)
	{
		this->chain=new label();
		expression *f=new expression(e,O_NOT);
		chain->generateCondJump(f);
	}
}

void ifStatement::close()
{
	logger("closing ifStatement");
	/* elsif ended without else in between */
	if(this->chain)error(E_BAD_SYNTAX);
	this->done->generate();
	logger("ifStatement closed");
}

ifStatement::~ifStatement()
{
	delete this->redo;
	delete this->done;
}

/* Loop definitions */
repeatLoop::repeatLoop():codeType(T_REPEATLOOP)
{
	this->loopStart=new label();
	this->loopEnd=new label();
	loopStart->generate();
}

void repeatLoop::generateBreak()
{
	loopEnd->generateJumpTo();
}

void repeatLoop::close(expression *e)
{
	expression *f=new expression(e, O_NOT);
	loopStart->generateCondJump(f);
	loopEnd->generate();
}

repeatLoop::~repeatLoop()
{
	delete loopStart;
	delete loopEnd;
}

doLoop::doLoop():codeType(T_DOLOOP)
{
	this->loopStart=new label();
	this->loopEnd=new label();
	loopStart->generate();
}

void doLoop::generateBreak()
{
	loopEnd->generateJumpTo();
}

void doLoop::close()
{
	this->loopStart->generateJumpTo();
	this->loopEnd->generate();
}

doLoop::~doLoop()
{
	delete loopStart;
	delete loopEnd;
}

whileLoop::whileLoop(expression *e):codeType(T_WHILELOOP)
{
	logger("creating whileLoop");
	loopContinue=new label();
	loopStart=new label();
	loopEnd=new label();
	cond=e;
	loopStart->generateJumpTo();
	loopContinue->generate();
	logger("whileLoop created");
}

void whileLoop::generateBreak()
{
	loopEnd->generateJumpTo();
}

void whileLoop::close()
{
	logger("closing whileLoop");
	loopStart->generate();
	loopContinue->generateCondJump(cond);
	loopEnd->generate();
	logger("whileLoop cleared");
}

whileLoop::~whileLoop() 
{
	delete loopContinue;
	delete loopStart;
	delete loopEnd;
}


forLoop::forLoop(variableType *v, expression *start, expression *stop,
	expression *stepVal):codeType(T_FORLOOP)
{
	logger("creating forLoop");
	startTemp=tempVar::getOrCreateVar(v->getSimpleVarType());
	stopTemp=tempVar::getOrCreateVar(v->getSimpleVarType());
	logger("forLoop assignments");
	/*v=start;
	stopTemp=stop;*/
	v->assignment(start);
	logger("start evaluated");
	stopTemp->assignment(stop);
	/* if (v<stopTemp) */
	logger("creating embedded if statement in forLoop");
	ifStatement *c=new ifStatement(new expression(new expression(v), O_LESS,
			new expression(stopTemp)));
	/* startTemp=v;*/
	startTemp->assignment(new expression(v));
	/* else */
	logger("ending then clause");
	c->alternative();
	/* startTemp=stopTemp;
	stopTemp=v;*/
	startTemp->assignment(new expression(stopTemp));
	stopTemp->assignment(new expression(v));
	/* endif */
	logger("ending else clause");
	c->close();
	delete c;
	logger("embedded if statement cleared");
	this->var=v;

	/* while (v<=stopTemp && v>=startTemp) */
	logger("generating range check for forLoop");
	expression *stopper1=new expression(new expression(v), O_LESS_EQUAL, 
		new expression(stopTemp));
	expression *stopper2=new expression(new expression(v), O_GREATER_EQUAL, 
		new expression(startTemp));
	expression *stopper=new expression(stopper1, O_AND, stopper2);
	logger("generating while portion of forLoop");
	infrastructure=new whileLoop(new expression(stopper, O_UNEQUAL,
		new expression(new constOp("0", T_INT))));
	if (stepVal)
	{
		logger("explicit step value");
		step=stepVal;
	}
	else
	{
		logger("implicit step value");
		/* if not present "step" is assumed to be 1 */
		step=new expression(new constOp("1", T_INT));
	}
	logger("forLoop definition cleared");
}

void forLoop::generateBreak()
{
	infrastructure->generateBreak();
}

void forLoop::close()
{
	logger("closing forLoop");
	/* var=var+step; */
	expression *stepper=new expression(new expression(var), O_PLUS, step);
	var->assignment(stepper);
	logger("step applied");
	infrastructure->close();
	startTemp->dispose();
	stopTemp->dispose();
	delete infrastructure;
	logger("forLoop closed");
}

forLoop::~forLoop()
{}
