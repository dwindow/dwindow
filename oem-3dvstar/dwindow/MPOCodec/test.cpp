#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include "mpoparser.h"
#include "3DPParser.h"

int main()
{

	_3DPParser parser;
	MpoParser parser2;
	int o = parser2.parseFile(L"Z:\\harvest.mpo");
	o = parser.parseFile(L"Z:\\harvest.mpo");
	o = parser.parseFile(L"Z:\\harvest.mpo");
	o = parser.parseFile(L"Z:\\harvest.mpo");
	o = parser.parseFile(L"Z:\\ppp.3dp");

	FILE *f1 = fopen("z:\\l1.jpg", "wb");
	FILE *f2 = fopen("z:\\l2.jpg", "wb");

	fwrite(parser2.m_datas[0], 1, parser2.m_sizes[0], f1);
	fwrite(parser2.m_datas[1], 1, parser2.m_sizes[1], f2);

	fclose(f1);
	fclose(f2);

	return 0;
}