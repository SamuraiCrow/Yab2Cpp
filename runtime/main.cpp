/*
**  Practice runtime for Yab2Cpp
**
**  by Samuel D. Crow
*/
#include "runtime.h"

int main(int argc, char *argv[])
{
	unsigned int ret=run();
	if (ret!=EXIT)
	{
		return 1;
	}
	return 0;
}
