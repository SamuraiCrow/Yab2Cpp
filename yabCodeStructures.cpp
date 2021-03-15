/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

/* base class of all the code structure types */
codeType::codeType(enum CODES t)
{
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

void label::dumpLabels()
{
	varNames << "Global Labels\n\n";
	for(auto iter=lookup.begin(); iter!=lookup.end(); ++iter)
	{
		varNames << "label " << iter->first << " has ID " << iter->second->getID() << "\n" ;
	}
	varNames << endl;
}

void label::generateJumpTo()
{
	output_cpp << "state=" << this->getID() << ";\nbreak;\n";
}

void label::generateOnNSkip(list<shared_ptr<label> >dest)
{
	if (dest->size()<2)
	{
		errorLevel=E_BAD_SYNTAX;
		exit(1);
	}
	auto iter=dest.start(); 
	consts_h << "j" << this->getID() << "[]={" << *iter;
	++iter;
	while(iter!=dest.end())
	{
		consts_h << ", " << *iter;
		++iter;
	}
	consts_h << "}\njs" << this->getID()<< "=" << dest->size() << ";\n";
}

void label::generateOnNTo(expression *e)
{
	operands *o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		output_cpp<< "if(" << o->boxName() << ">=0 && "
		    << o->boxName() << "<js" << this->getID() << ")state=j["
		    << o->boxName() << "];\nbreak;\n";
	}
	delete e;
}

void label::generateCondJump(expression *e)
{
	operands *o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		output_cpp<< "if(" << o->boxName() 
            << "!=0)state=" << this->getID() << ";\nbreak;\n";
	}
	delete e;
}

void label::generate()
{
	output_cpp<< "case " << this->getID() <<":\n";
}

/* conditional definition */

conditional::conditional(expression *e):codeType(T_IF)
{
	this->redo=new label();
	redo->generate();
	this->done=new label();
	expression *f=new expression(e,O_NOT);
	this->chain=new label();
	chain->generateCondJump(f);
}

void conditional::generateBreak()
{
	done->generateJumpTo();
}

void conditional::generateContinue()
{
	redo->generateJumpTo();
}

void conditional::alternative(expression *e=NULL)
{
	done->generateJumpTo();
	this->chain->generate();
	delete this->chain;
	this->chain=NULL;
	if(e!=NULL)
	{
		this->chain=new label();
		expression *f=new expression(e,O_NOT);
		chain->generateJumpCond(f);
	}
}

void conditional::close()
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
{	delete loopStart;
	delete loopEnd;
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
	loopContinue->generateJumpCond(cond);
	loopEnd->generate();
}

whileLoop::~whileLoop()
{
	delete loopStart;
	delete loopContinue;
	delete loopEnd;
}

forLoop::forLoop(variable *v, expression *start, expression *stop, expression *stepVal=NULL):codeType(T_FORLOOP)
{
	/*v=start;
	stopTemp=stop;*/
	v->assignment(start);
	stopTemp->assignment(stop);
	/* if (v<stopTemp) */
	conditional *c=new conditional(new expression(new expression(v), O_LESS, new expression(stopTemp)));
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
	delete c;
	/* while (v<=stopTemp && v>=startTemp) */
	expression *stopper1=new expression(new expression(v), O_LESS_EQUAL, new expression(stopTemp));
	expression *stopper2=new expression(new expression(v), O_GREATER_EQUAL, new expression(startTemp));
	expression *stopper=new expression(stopper1, O_AND, stopper2);
	this->infrastructure=new whileLoop(new expression(stopper, O_UNEQUAL, 
		new expression(operands::createConst("0", T_INT))));
	if (stepVal)
	{
		step=stepVal;
	}
	else
	{
		/* if not present "step" is assumed to be 1 */
		step=new expression(operands::createConst("1", T_INT));
	}
}

void forLoop::generateBreak()
{
	infrastructure->generateBreak();
}

void forLoop::close()
{
	/* v=v+step; */
	expression *stepper=new expression(new expression(v), O_PLUS, step);
	v->assignment(stepper)
	infrastructure->close();
}
