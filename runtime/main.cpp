/*
**  Practice runtime for Yab2Cpp
**
**  by Samuel D. Crow
*/
#include "runtime.h"

subroutine *callStack=nullptr;

subroutine::subroutine(unsigned int r)
{
	this->ret=r;
	this->called=callStack;
}

unsigned int subroutine::close()
{
	if (callStack==nullptr) return STACK_UNDERFLOW_ERROR;
	unsigned int r=callStack->ret;
	subroutine *l=callStack->called;
	delete callStack;
	callStack=l;
	return r;
}

int main(int argc, char *argv[])
{
	unsigned int ret=run();
	if (ret!=EXIT)
	{
		return 1;
	}
	return 0;
}
