#ifndef _MODULES_H_
#define _MODULES_H_

#include <stddef.h>
#include <stdint.h>
#include "Lists.h"

namespace Modules {

struct DynamicEntry {
	size_t kind;
	size_t val;
};

struct Symbol {
	uint32_t nameIdx;
	uint8_t info;
	uint8_t vis;
	uint16_t sect;
	size_t val;
	size_t size;
};

class Module: public Lists::List {
public:
	char fName[256];
	const DynamicEntry *fDynamic;
	const char *fNames;
	size_t fSymbolCnt;
	const Symbol *fSymbols;

	Module(const char *name, const DynamicEntry *dynamic);
	void *ThisSym(const char *name);
	const char *SymAt(void *adr, size_t *ofs = NULL, size_t *size = NULL);
	
};

void WritePC(size_t pc);
void StackTrace();
void Init();
Module *Load(const char *name);
void Unload(Module *mod);
Module *Loaded(const char *name);

}

#endif	// _MODULES_H_
