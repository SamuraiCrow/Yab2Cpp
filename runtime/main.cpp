/*
**  Practice runtime for Yab2Cpp
**
**  by Samuel D. Crow
*/
#include "runtime.h"
#include <cstdio>

subroutine *callStack=nullptr;

subroutine::subroutine(unsigned int r)
{
	this->ret=r;
	this->callStackNode=callStack;
}

unsigned int subroutine::close()
{
	if (callStack==nullptr) return STACK_UNDERFLOW_ERROR;
	unsigned int r=callStack->ret;
	subroutine *l=callStack->callStackNode;
	delete callStack;
	callStack=l;
	return r;
}

int main(int argc, char *argv[])
{
	unsigned int ret=run();
	switch (ret) {
		case STACK_UNDERFLOW_ERROR:
			puts("Stack Underflow\n");
			break;
		case UNDEFINED_STATE_ERROR:
			puts("Program encountered an undefined state\n");
			break;
		case EXIT:
			return 0;
		default:
			puts("Illegal fallthrough encountered\n");
			break;
	}
	return 1;
}
