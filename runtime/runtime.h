/*
**  Practice runtime header for Yab2Cpp
**
**  by Samuel D. Crow
*/
#ifndef YAB_RUNTIME
#define YAB_RUNTIME

#include <string>
#include <cstdio>
using namespace std;

enum STATES:unsigned int
{
    EXIT,
    UNDEFINED_STATE_ERROR,
    STACK_UNDERFLOW_ERROR,
    START
};

class subroutine
{
    subroutine *called;
    unsigned int ret;

public:
    static unsigned int close();
    subroutine(unsigned int r);
    virtual ~subroutine()
    {}
};

extern subroutine *callStack;

/* function prototype */
unsigned int run();

#endif
