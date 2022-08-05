/*
**  Runtime header for Yab2Cpp
**
**  by Samuel D. Crow
*/
#ifndef YAB_RUNTIME
#define YAB_RUNTIME

#include <string>
#include <cstdio>
using namespace std;

/*
This enum contains all of the error states used at runtime.
*/
enum STATES:unsigned int
{
    EXIT,
    UNDEFINED_STATE_ERROR,
    STACK_UNDERFLOW_ERROR,
    OUT_OF_RANGE,
    START
};

/*
This class wraps the function class and is inherited by every subroutine.
*/
class subroutine
{
    subroutine *callStackNode;
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
