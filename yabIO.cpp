/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	based on Yab
**
*/
#include "yab2cpp.h"

/* prototypes for local functions */
string formatString(enum SEPARATORS s);
string formatInt(enum SEPARATORS s);
string formatFloat(enum SEPARATORS s);

printSegment::printSegment(expression *e, enum SEPARATORS s)
{
	cargo=e;
	sep=s;
}

void printSegment::generate()
{
	if (cargo!=NULL)
	{
		operands *op=cargo->evaluate();
		switch (op->getSimpleVarType())
		{
		case T_STRINGVAR:
			output_cpp << "printf(\"" << formatString(sep) << "\"," 
				<< op->boxName() << ".c_str());\n";
			break;
		case T_INTVAR:
			output_cpp << "printf(\"" << formatInt(sep) << "\", "
				<< op->boxName() << ");\n";
			break;
		case T_FLOATVAR:
			output_cpp << "printf(\"" << formatFloat(sep) << "\", " 
				<< op->boxName() << ");\n";
			break;
		default:
			error(E_TYPE_MISMATCH);
			break;
		}
		return;
	}
	switch (sep) {
		case S_LINEFEED:
			output_cpp << "puts(\"\\n\");\n";
			break;
		case S_COMMA:
			output_cpp << "putchar(\'\\t\');\n";
			break;
		case S_SEMICOLON:
			break;
		default:
			error(E_INTERNAL);
			break;
	}
}

string formatString(enum SEPARATORS s)
{
	switch (s)
	{
	case S_LINEFEED:
		return string("%s\\n");
	case S_COMMA:
		return string("%s\\t");
	case S_SEMICOLON:
		return string("%s");
	default:
		error(E_BAD_SYNTAX);
		return string("");
	}
}

string formatInt(enum SEPARATORS s)
{
	switch (s)
	{
	case S_LINEFEED:
		return string("%d\\n");
	case S_COMMA:
		return string("%d\\t");
	case S_SEMICOLON:
		return string("%d");
	default:
		error(E_BAD_SYNTAX);
		return string("");
	}
}

string formatFloat(enum SEPARATORS s)
{
	switch (s)
	{
	case S_LINEFEED:
		return string("%f\\n");
	case S_COMMA:
		return string("%f\\t");
	case S_SEMICOLON:
		return string("%f");
	default:
		error(E_BAD_SYNTAX);
		return string("");
	}
}
