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
	varNames << "Global Labels\n\n";
	for(auto iter=lookup.begin(); iter!=lookup.end(); ++iter)
	{
		varNames << "label " << iter->first << " has ID "
			<< iter->second->getID() << "\n" ;
	}
	varNames << endl;
}

void label::generateEnd()
{
	output_cpp << "state=EXIT;\nbreak;\n";
}

void label::generateJumpTo()
{
	output_cpp << "state=" << this->getID() << ";\nbreak;\n";
}

/* pass this as second parameter to generateOnNTo or generateOnNSub */
unsigned int label::generateOnNSkip(list<label *>&dest)
{
	if (dest.size()<2)error(E_BAD_SYNTAX);
	auto iter=dest.begin();
	consts_h << "j" << this->getID() << "[]={" << *iter;
	++iter;
	while(iter!=dest.end())
	{
		consts_h << ", " << *iter;
		++iter;
	}
	consts_h << "}\njs" << this->getID()<< "=" << dest.size() << ";\n";
	return this->getID();
}

void label::generateOnNTo(expression *e, unsigned int skip)
{
	operands *o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		output_cpp<< "if(" << o->boxName() << ">=0 && "
		    << o->boxName() << "<js" << skip << ")state=j["
		    << o->boxName() << "];\nbreak;\n";
		o->dispose();
		delete e;
		return;
	}
	error(E_TYPE_MISMATCH);
}

void label::generateCondJump(expression *e)
{
	operands *o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		output_cpp<< "if(" << o->boxName() 
            << "!=0)state=" << this->getID() << ";\nbreak;\n";
		o->dispose();
		delete e;
		return;
	}
	error(E_TYPE_MISMATCH);
}

void label::generate()
{
	output_cpp<< "case " << this->getID() <<":\n";
}

/* ifStatement definition */

ifStatement::ifStatement(expression *e):codeType(T_IF)
{
	this->redo=new label();
	redo->generate();
	this->done=new label();
	expression *f=new expression(e,O_NOT);
	this->chain=new label();
	chain->generateCondJump(f);
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
	/* elsif ended without else in between */
	if(this->chain)error(E_BAD_SYNTAX);
	this->done->generate();
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

whileLoop::whileLoop(expression *e):codeType(T_WHILELOOP)
{
	loopContinue=new label();
	loopStart=new label();
	loopEnd=new label();
	cond=e;
	loopStart->generateJumpTo();
	loopContinue->generate();
}

void whileLoop::generateBreak()
{
	loopEnd->generateJumpTo();
}

void whileLoop::close()
{
	loopStart->generate();
	loopContinue->generateCondJump(cond);
	loopEnd->generate();
}

forLoop::forLoop(variableType *v, expression *start, expression *stop,
	expression *stepVal):codeType(T_FORLOOP)
{
	/*v=start;
	stopTemp=stop;*/
	v->assignment(start);
	stopTemp->assignment(stop);
	/* if (v<stopTemp) */
	ifStatement *c=new ifStatement(new expression(new expression(v), O_LESS,
			new expression(stopTemp)));
	/* startTemp=v;*/
	startTemp->assignment(new expression(v));
	/* else */
	c->alternative();
	/* startTemp=stopTemp;
	stopTemp=v;*/
	startTemp->assignment(new expression(stopTemp));
	stopTemp->assignment(new expression(v));
	/* endif */
	c->close();
	/* while (v<=stopTemp && v>=startTemp) */
	expression *stopper1=new expression(new expression(v), O_LESS_EQUAL, 
		new expression(stopTemp));
	expression *stopper2=new expression(new expression(v), O_GREATER_EQUAL, 
		new expression(startTemp));
	expression *stopper=new expression(stopper1, O_AND, stopper2);
	infrastructure=new whileLoop(new expression(stopper, O_UNEQUAL,
		new expression(new constOp("0", T_INT))));
	if (stepVal)
	{
		step=stepVal;
	}
	else
	{
		/* if not present "step" is assumed to be 1 */
		step=new expression(new constOp("1", T_INT));
	}
}

void forLoop::generateBreak()
{
	infrastructure->generateBreak();
}

void forLoop::close()
{
	/* var=var+step; */
	expression *stepper=new expression(new expression(var), O_PLUS, step);
	var->assignment(stepper);
	infrastructure->close();
}
