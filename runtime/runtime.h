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
    struct subroutine *called;
    enum STATES ret;

public:
    static enum STATES close();
    subroutine(enum STATES r);
    virtual ~subroutine()
    {}
};

extern struct subroutine *callStack;

/* function prototype */
unsigned int run();

#endif
