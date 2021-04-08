/*
**  Practice runtime for Yab2Cpp
**
**  by Samuel D. Crow
*/
#include "runtime.h"

struct subroutine *callStack=nullptr;

subroutine::subroutine(enum STATES r)
{
	this->ret=r;
	this->called=callStack;
}

enum STATES subroutine::close()
{
	if (callStack==nullptr) return STACK_UNDERFLOW_ERROR;
	enum STATES r=callStack->ret;
	struct subroutine *l=callStack->called;
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
