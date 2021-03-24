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
unordered_map<string, shared_ptr<label> > label::lookup;
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
shared_ptr<label>label::find(string &s)
{
	unordered_map<string, shared_ptr<label> >lookup;
	auto ret=label::lookup.find(s);
	return(ret==label::lookup.end()?NULL:ret->second);
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
unsigned int label::generateOnNSkip(list<shared_ptr<label> >&dest)
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

void label::generateOnNTo(shared_ptr<expression>e, unsigned int skip)
{
	shared_ptr<operands>o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		output_cpp<< "if(" << o->boxName() << ">=0 && "
		    << o->boxName() << "<js" << skip << ")state=j["
		    << o->boxName() << "];\nbreak;\n";
	}
}

void label::generateCondJump(shared_ptr<expression>e)
{
	shared_ptr<operands>o=e->evaluate();
	if (o->getType()==T_INT||o->getType()==T_INTVAR)
	{
		output_cpp<< "if(" << o->boxName() 
            << "!=0)state=" << this->getID() << ";\nbreak;\n";
	}
}

void label::generate()
{
	output_cpp<< "case " << this->getID() <<":\n";
}

/* ifStatement definition */

ifStatement::ifStatement(shared_ptr<expression>e):codeType(T_IF)
{
	this->redo=shared_ptr<label>(new label());
	redo->generate();
	this->done=shared_ptr<label>(new label());
	shared_ptr<expression>f=shared_ptr<expression>(new expression(e,O_NOT));
	this->chain=shared_ptr<label>(new label());
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

void ifStatement::alternative(shared_ptr<expression>e)
{
	done->generateJumpTo();
	this->chain->generate();
	this->chain=NULL;
	if(e!=NULL)
	{
		this->chain=shared_ptr<label>(new label());
		shared_ptr<expression>f=shared_ptr<expression>(
			new expression(e,O_NOT));
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
	this->loopStart=shared_ptr<label>(new label());
	this->loopEnd=shared_ptr<label>(new label());
	loopStart->generate();
}

void repeatLoop::generateBreak()
{
	loopEnd->generateJumpTo();
}

void repeatLoop::close(shared_ptr<expression>e)
{
	shared_ptr<expression>f=shared_ptr<expression>(new expression(e, O_NOT));
	loopStart->generateCondJump(f);
	loopEnd->generate();
}

doLoop::doLoop():codeType(T_DOLOOP)
{
	this->loopStart=shared_ptr<label>(new label());
	this->loopEnd=shared_ptr<label>(new label());
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

whileLoop::whileLoop(shared_ptr<expression>e):codeType(T_WHILELOOP)
{
	loopContinue=shared_ptr<label>(new label());
	loopStart=shared_ptr<label>(new label());
	loopEnd=shared_ptr<label>(new label());
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

forLoop::forLoop(shared_ptr<variableType>v,
	shared_ptr<expression>start, shared_ptr<expression>stop,
	shared_ptr<expression>stepVal):codeType(T_FORLOOP)
{
	/*v=start;
	stopTemp=stop;*/
	v->assignment(start);
	stopTemp->assignment(stop);
	/* if (v<stopTemp) */
	shared_ptr<ifStatement>c=shared_ptr<ifStatement>(new ifStatement(
		shared_ptr<expression>(new expression(shared_ptr<expression>(
			new expression(v)),	O_LESS,	shared_ptr<expression>(
			new expression(stopTemp))))));
	/* startTemp=v;*/
	startTemp->assignment(shared_ptr<expression>(new expression(v)));
	/* else */
	c->alternative();
	/* startTemp=stopTemp;
	stopTemp=v;*/
	startTemp->assignment(shared_ptr<expression>(new expression(stopTemp)));
	stopTemp->assignment(shared_ptr<expression>(new expression(v)));
	/* endif */
	c->close();
	/* while (v<=stopTemp && v>=startTemp) */
	shared_ptr<expression>stopper1=shared_ptr<expression>(new expression(
		shared_ptr<expression>(new expression(v)), O_LESS_EQUAL, 
		shared_ptr<expression>(new expression(stopTemp))));
	shared_ptr<expression>stopper2=shared_ptr<expression>(new expression(
		shared_ptr<expression>(new expression(v)), O_GREATER_EQUAL, 
		shared_ptr<expression>(new expression(startTemp))));
	shared_ptr<expression>stopper=shared_ptr<expression>(new expression(
		stopper1, O_AND, stopper2));
	shared_ptr<whileLoop>infrastructure=shared_ptr<whileLoop>(new whileLoop(
		shared_ptr<expression>(new expression(stopper, O_UNEQUAL,
		shared_ptr<expression>(new expression(
		shared_ptr<constOp>(new constOp("0", T_INT))))))));
	if (stepVal)
	{
		step=stepVal;
	}
	else
	{
		/* if not present "step" is assumed to be 1 */
		step=shared_ptr<expression>(new expression(
			shared_ptr<constOp>(new constOp("1", T_INT))));
	}
}

void forLoop::generateBreak()
{
	infrastructure->generateBreak();
}

void forLoop::close()
{
	/* var=var+step; */
	shared_ptr<expression>stepper=shared_ptr<expression>(new expression(
		shared_ptr<expression>(new expression(var)), O_PLUS, step));
	var->assignment(stepper);
	infrastructure->close();
}
