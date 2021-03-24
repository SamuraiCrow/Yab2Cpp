/*
**  Practice runtime header for Yab2Cpp
**
**  by Samuel D. Crow
*/
#ifndef YAB_RUNTIME
#define YAB_RUTTIME

#include <cstdio>
using namespace std;

enum STATES:unsigned int
{
    EXIT,
    UNDEFINED_STATE_ERROR,
    START
};

/* function prototype */
unsigned int run();

#endif
