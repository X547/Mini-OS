#ifndef _LISTS_H_
#define _LISTS_H_

#include <stddef.h>

namespace Lists {

struct List {
	List *fPrev, *fNext;

	static inline void Join(List *a, List *b)
	{
		a->fNext = b; b->fPrev = a;
	}

	void InsertBefore(List *it)
	{
		Join(fPrev, it); Join(it, this);
	}

	void InsertAfter(List *it)
	{
		Join(it, fNext); Join(this, it);
	}

	void Remove(List *it)
	{
		Join(it->fPrev, it->fNext);
		it->fPrev = NULL; it->fNext = NULL;
	}

};

}

#endif	// _LISTS_H_
