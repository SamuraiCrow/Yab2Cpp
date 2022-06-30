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
const char *formatString(enum SEPARATORS s);
const char *formatInt(enum SEPARATORS s);
const char *formatFloat(enum SEPARATORS s);

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

const char *formatString(enum SEPARATORS s)
{
	switch (s)
	{
	case S_LINEFEED:
		return "%s\\n";
	case S_COMMA:
		return "%s\\t";
	case S_SEMICOLON:
		return "%s";
	default:
		error(E_BAD_SYNTAX);
		return "";
	}
}

const char *formatInt(enum SEPARATORS s)
{
	switch (s)
	{
	case S_LINEFEED:
		return "%d\\n";
	case S_COMMA:
		return "%d\\t";
	case S_SEMICOLON:
		return "%d";
	default:
		error(E_BAD_SYNTAX);
		return "";
	}
}

const char *formatFloat(enum SEPARATORS s)
{
	switch (s)
	{
	case S_LINEFEED:
		return "%f\\n";
	case S_COMMA:
		return "%f\\t";
	case S_SEMICOLON:
		return "%f";
	default:
		error(E_BAD_SYNTAX);
		return "";
	}
}
