/*
**	Yab2Cpp
**
**	Transpiler by Samuel D. Crow
**
**	based on Yab
**
*/
#include "yab2cpp.h"

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
			output_cpp << "puts(" << op->boxName() << ");\n";
			break;
		case T_INTVAR:
			output_cpp << "printf(\"%d\", " << op->boxName() << ");\n";
			break;
		case T_FLOATVAR:
			output_cpp << "printf(\"%f\", " << op->boxName() << ");\n";
			break;
		default:
			error(E_TYPE_MISMATCH);
			break;
		}
	}
	switch (sep)
	{
	case S_LINEFEED:
		output_cpp << "puts(\"\\n\");\n";
		return;
	case S_SEMICOLON:
		return;
	case S_COMMA:
		output_cpp << "putc('\\t');\n";
		return;
	default:
		error(E_BAD_SYNTAX);
		break;
	}
}
