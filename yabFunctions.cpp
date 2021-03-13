/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	Based on Yab
**
*/
#include "yab2cpp.h"

/* function definitions */
fn *fn::getCurrentSub()
{
	return callStack.back;
}

void fn::generateOnNSub(expression *e)
{
	this->ret=new label();
	fn::callStack.push_back(this);
	label::generateOnNTo(e);
	output_cpp << "case " << ret->getID() << ":\n";
}

void fn::generateGosub(shared_ptr<label> sub)
{
	this->ret=new label();
	fn::callStack.push_back(this);
	output_cpp << "state=" << sub->getID() << ";\nbreak;\n";
	output_cpp << "case " << ret->getID() << ":\n";
}
