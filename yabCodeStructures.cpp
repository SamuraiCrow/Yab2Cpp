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
	this->id= ++nextID;
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

void label::dumpLabels(ostream &v)
{
	v << "Global Labels\n\n";
	for(auto iter=lookup.begin(); iter!=lookup.end(); ++iter)
	{
		v << "label " << iter->first << " has ID " << iter->second->getID() << "\n" ;
	}
	v << endl;
}

void label::generateJumpTo(ostream &out)
{
	out << "state=" << this->getID() << ";\nbreak;\n";
}

void label::generateOnNSkip(ostream &k, list<label *> &dest)
{
	if (dest->size()<2)
	{
		errorLevel=E_BAD_SYNTAX;
		exit(1);
	}
	auto iter=dest.start(); 
	k << "j" << this->getID() << "[]={" << *iter;
	++iter;
	while(iter!=dest.end())
	{
		k << ", " << *iter;
		++iter;
	}
	k << "}\njs" << this->getID()<< "=" << dest->size() << ";\n";
}

void label::generateOnNTo(ostream &out, expression *e)
{
	operands *o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		out << "if(";
		o->boxName(out);
		out << ">=0 && ";
		o->boxName(out);
		out << "<js" << this->getID() << ")state=j["; 
		o->boxName(out);
		out << "];\nbreak;\n";
	}
	delete e;
}

void label::generateCondJump(ostream &out, expression *e)
{
	operands *o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		out << "if(";
		o->boxName(out);
		out << "!=0)state=" << this->getID() << ";\nbreak;\n";
	}
	delete e;
}

void label::generate(ostream &out)
{
	out << "case " << this->getID() <<":\n";
}

/* conditional definition */

conditional::conditional(ostream &out, expression *e):codeType(T_IF)
{
	this->redo=new label();
	redo->generate(out);
	this->done=new label();
	expression *f=new expression(e,O_NOT);
	this->chain=new label();
	chain->generateCondJump(out, f);
}

void conditional::generateBreak(ostream &out)
{
	done->generateJumpTo(out);
}

void conditional::generateContinue(ostream &out)
{
	redo->generateJumpTo(out);
}

void conditional::alternative(ostream &out, expression *e=NULL)
{
	done->generateJumpTo(out);
	this->chain->generate();
	delete this->chain;
	this->chain=NULL;
	if(e!=NULL)
	{
		this->chain=new label();
		expression *f=new expression(e,O_NOT);
		chain->generateJumpCond(out, f);
	}
}

void conditional::close(ostream &out)
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
repeatLoop::repeatLoop(ostream &out):codeType(T_REPEATLOOP)
{
	this->loopStart=new label();
	this->loopEnd=new label();
	loopStart->generate(out;)
}

void repeatLoop::generateBreak(ostream &out)
{
	loopEnd->generateJumpTo(out);
}

void repeatLoop::close(ostream &out, expression *e)
{
	expression *f=new expression(e, O_NOT);
	loopStart->generateCondJump(out, f);
	loopEnd->generate(out);
}

repeatLoop::~repeatLoop()
{
	delete loopStart;
	delete loopEnd;
}

doLoop::doLoop(ostream &out):codeType(T_DOLOOP)
{
	this->loopStart=new label();
	this->loopEnd=new label();
	loopStart->generate(out;)
}

void doLoop::generateBreak(ostream &out)
{
	loopEnd->generateJumpTo(out);
}

void doLoop::close(ostream &out)
{
	this->loopStart->generateJumpTo(out);
	this->loopEnd->generate(out);
}

doLoop::~doLoop()
{	delete loopStart;
	delete loopEnd;
}

whileLoop::whileLoop(ostream &out, expression *e):codeType(T_WHILELOOP)
{
	loopContinue=new label();
	loopStart=new label();
	loopEnd=new label();
	cond=e;
	loopStart->generateJumpTo(out);
	loopContinue->generate(out);
}

void whileLoop::generateBreak(ostream &out)
{
	loopEnd->generateJumpTo(out);
}

void whileLoop::close(ostream &out)
{
	loopStart->generate(out);
	loopContinue->generateJumpCond(out, cond);
	loopEnd->generate(out);
}

whileLoop::~whileLoop()
{
	delete loopStart;
	delete loopContinue;
	delete loopEnd;
}

forLoop::forLoop(ostream &out, ostream &k, variable *v, expression *start, expression *stop, expression *stepVal=NULL):codeType(T_FORLOOP)
{
	/*v=start;
	stopTemp=stop;*/
	v->assignment(out, start);
	stopTemp->assignment(out, stop);
	/* if (v<stopTemp) */
	conditional *c=new conditional(out, new expression(new expression(v), O_LESS, new expression(stopTemp)));
	/* startTemp=v;*/
	startTemp->assignment(out, new expression(v));
	/* else */
	c->alternative(out);
	/* startTemp=stopTemp;
	stopTemp=v;*/
	startTemp->assignment(out, new expression(stopTemp));
	stopTemp->assignment(out, new expression(v));
	/* endif */
	c->close(out);
	delete c;
	/* while (v<=stopTemp && v>=startTemp) */
	expression *stopper1=new expression(new expression(v), O_LESS_EQUAL, new expression(stopTemp));
	expression *stopper2=new expression(new expression(v), O_GREATER_EQUAL, new expression(startTemp));
	expression *stopper=new expression(stopper1, O_AND, stopper2);
	this->infrastructure=new whileLoop(out, new expression(stopper, O_UNEQUAL, 
		new expression(operands::createConst(k, "0", T_INT))));
	if (stepVal)
	{
		step=stepVal;
	}
	else
	{
		/* if not present "step" is assumed to be 1 */
		step=new expression(operands::createConst(k, "1", T_INT));
	}
}

void forLoop::generateBreak(ostream &out)
{
	infrastructure->generateBreak(out);
}

void forLoop::close(ostream &out)
{
	/* v=v+step; */
	expression *stepper=new expression(new expression(v), O_PLUS, step);
	v->assignment(out, stepper)
	infrastructure->close(ostream &out);
}
