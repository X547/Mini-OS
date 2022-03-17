#include "Modules.h"
#include "Graphics.h"
#include "RiscV.h"
#include <string.h>
#include <new>

extern Modules::DynamicEntry startupDynamic;

namespace Modules {

Lists::List gModules = {&gModules, &gModules};

Module::Module(const char *name, const DynamicEntry *dynamic):
	fDynamic(dynamic),
	fNames(NULL),
	fSymbolCnt(0),
	fSymbols(NULL)
{
	strlcpy(fName, name, sizeof(fName)/sizeof(fName[0]));
	// WriteString("Dynamic table"); WriteLn();
	for (size_t i = 0; fDynamic[i].kind != 0; i++) {
		// WriteTab(); WriteInt(fDynamic[i].kind); WriteString(": "); WriteHex(fDynamic[i].val, 8); WriteLn();
		switch (fDynamic[i].kind) {
		case 4: fSymbolCnt = *((uint32_t*)fDynamic[i].val + 1); break;
		case 5: fNames = (const char*)fDynamic[i].val; break;
		case 6: fSymbols = (const Symbol*)fDynamic[i].val; break;
		default: ;
		}
	}
/*
	WriteString("Module "); WriteString(fName); WriteLn();
	WriteTab(); WriteString("fDynamic: "); WriteHex((size_t)fDynamic, 8); WriteLn();
	WriteTab(); WriteString("fSymbolCnt: "); WriteInt(fSymbolCnt); WriteLn();
	WriteTab(); WriteString("fNames: "); WriteHex((size_t)fNames, 8); WriteLn();
	WriteTab(); WriteString("fSymbols: "); WriteHex((size_t)fSymbols, 8); WriteLn();
*/
}

const char *Module::SymAt(void *adr, size_t *ofs, size_t *size)
{
	for (size_t i = 0; i < fSymbolCnt; i++) {
		if ((size_t)adr - fSymbols[i].val >= 0 && (size_t)adr - fSymbols[i].val < fSymbols[i].size) {
			if (ofs != NULL) *ofs = (size_t)adr - fSymbols[i].val;
			if (size != NULL) *size = fSymbols[i].size;
			return fNames + fSymbols[i].nameIdx;
		}
	}
	return NULL;
}


void WritePC(size_t pc)
{
	Module *mod = Loaded("Startup");
	if (mod != NULL) {
		size_t ofs;
		const char *name = mod->SymAt((void*)pc, &ofs);
		if (name != NULL) {
			WriteString(name); WriteString(" + "); WriteInt(ofs);
			return;
		}
	}
	WriteHex(pc, 8);
}

void StackTrace()
{
	WriteString("Stack:"); WriteLn();
	size_t fp = Fp(), pc;
	while (fp != 0) {
		pc = *((size_t*)fp - 1);
		fp = *((size_t*)fp - 2);
		WriteTab(); WriteString("FP: "); WriteHex(fp, 8);
		WriteString(", PC: "); WritePC(pc - 1); WriteLn();
	}
}

Module *Loaded(const char *name)
{
	for (Module *it = (Module*)gModules.fNext; it != &gModules; it = (Module*)it->fNext) {
		if (strcmp(it->fName, name) == 0)
			return it;
	}
	return NULL;
}

void Init()
{
	Module *mod = new(std::nothrow) Module("Startup", &startupDynamic);
	gModules.InsertBefore(mod);
}

}
