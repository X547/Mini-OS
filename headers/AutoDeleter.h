/*
 * Copyright 2001-2007, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUTO_DELETER_H
#define _AUTO_DELETER_H


/*!	Scope-based automatic deletion of objects/arrays.
	ObjectDeleter  - deletes an object
	ArrayDeleter   - deletes an array
	MemoryDeleter  - free()s malloc()ed memory
	CObjectDeleter - calls an arbitrary specified destructor function
	FieldFunctionDeleter - calls an arbitrary field function in given struct
		pointer
	HandleDeleter  - use arbitrary handle type and destructor function
*/

#include <malloc.h>
#include <string.h>


// AutoDeleter

template<typename C, typename DeleteFunc>
class AutoDeleter {
public:
	inline AutoDeleter()
		: fObject(NULL)
	{
	}

	inline AutoDeleter(C *object)
		: fObject(object)
	{
	}

	inline ~AutoDeleter()
	{
		DeleteFunc destructor;
		destructor(fObject);
	}

	inline void SetTo(C *object)
	{
		if (object != fObject) {
			DeleteFunc destructor;
			destructor(fObject);
			fObject = object;
		}
	}

	inline void Unset()
	{
		SetTo(NULL);
	}

	inline void Delete()
	{
		SetTo(NULL);
	}

	inline bool IsSet() const
	{
		return fObject != NULL;
	}

	inline C *Get() const
	{
		return fObject;
	}

	inline C *Detach()
	{
		C *object = fObject;
		fObject = NULL;
		return object;
	}

	inline C *operator->() const
	{
		return fObject;
	}

protected:
	C			*fObject;

private:
	AutoDeleter(const AutoDeleter&);
	AutoDeleter& operator=(const AutoDeleter&);
};


// ObjectDeleter

template<typename C>
struct ObjectDelete
{
	inline void operator()(C *object)
	{
		delete object;
	}
};

template<typename C>
struct ObjectDeleter : AutoDeleter<C, ObjectDelete<C> >
{
	ObjectDeleter() : AutoDeleter<C, ObjectDelete<C> >() {}
	ObjectDeleter(C *object) : AutoDeleter<C, ObjectDelete<C> >(object) {}
};


// ArrayDeleter

template<typename C>
struct ArrayDelete
{
	inline void operator()(C *array)
	{
		delete[] array;
	}
};

template<typename C>
struct ArrayDeleter : AutoDeleter<C, ArrayDelete<C> >
{
	ArrayDeleter() : AutoDeleter<C, ArrayDelete<C> >() {}
	ArrayDeleter(C *array) : AutoDeleter<C, ArrayDelete<C> >(array) {}

	inline C& operator[](size_t index) const
	{
		return this->Get()[index];
	}
};


// MemoryDeleter

struct MemoryDelete
{
	inline void operator()(void *memory)
	{
		free(memory);
	}
};

struct MemoryDeleter : AutoDeleter<void, MemoryDelete >
{
	MemoryDeleter() : AutoDeleter<void, MemoryDelete >() {}
	MemoryDeleter(void *memory) : AutoDeleter<void, MemoryDelete >(memory) {}
};


// CObjectDeleter

template<typename Type, typename DestructorReturnType,
	DestructorReturnType (*Destructor)(Type*)>
struct CObjectDelete
{
	inline void operator()(Type *object)
	{
		if (object != NULL)
			Destructor(object);
	}
};

template<typename Type, typename DestructorReturnType,
	DestructorReturnType (*Destructor)(Type*)>
struct CObjectDeleter
	: AutoDeleter<Type, CObjectDelete<Type, DestructorReturnType, Destructor> >
{
	typedef AutoDeleter<Type,
		CObjectDelete<Type, DestructorReturnType, Destructor> > Base;

	CObjectDeleter() : Base()
	{
	}

	CObjectDeleter(Type *object) : Base(object)
	{
	}
};


// MethodDeleter

template<typename Type, typename DestructorReturnType,
	DestructorReturnType (Type::*Destructor)()>
struct MethodDelete
{
	inline void operator()(Type *object)
	{
		if (object != NULL)
			(object->*Destructor)();
	}
};


template<typename Type, typename DestructorReturnType,
	DestructorReturnType (Type::*Destructor)()>
struct MethodDeleter
	: AutoDeleter<Type, MethodDelete<Type, DestructorReturnType, Destructor> >
{
	typedef AutoDeleter<Type,
		MethodDelete<Type, DestructorReturnType, Destructor> > Base;

	MethodDeleter() : Base()
	{
	}

	MethodDeleter(Type *object) : Base(object)
	{
	}
};


// FieldFunctionDeleter

template<typename Type, typename Table, Table **table,
	void (*Table::*Deleter)(Type*)>
struct FieldFunctionDelete {
	inline void operator()(Type *object)
	{
		if (object != NULL)
			((**table).*Deleter)(object);
	}
};

template<typename Type, typename Table, Table **table,
	typename DestructorResult, DestructorResult (*Table::*Deleter)(Type*)>
struct FieldFunctionDeleter
	: AutoDeleter<Type, FieldFunctionDelete<Type, Table, table, Deleter> >
{
	typedef AutoDeleter<Type,
		FieldFunctionDelete<Type, Table, table, Deleter> > Base;

	FieldFunctionDeleter() : Base() {}
	FieldFunctionDeleter(Type *object) : Base(object) {}
};


// HandleDeleter

struct StatusHandleChecker
{
	inline bool operator()(int handle)
	{
		return handle >= 0;
	}
};

template<typename C, typename DestructorResult,
	DestructorResult (*Destructor)(C), C nullValue = -1,
	typename Checker = StatusHandleChecker>
class HandleDeleter {
public:
	inline HandleDeleter()
		: fHandle(nullValue)
	{
	}

	inline HandleDeleter(C handle)
		: fHandle(handle)
	{
	}

	inline ~HandleDeleter()
	{
		if (IsSet())
			Destructor(fHandle);
	}

	inline void SetTo(C handle)
	{
		if (handle != fHandle) {
			if (IsSet())
				Destructor(fHandle);
			fHandle = handle;
		}
	}

	inline void Unset()
	{
		SetTo(nullValue);
	}

	inline void Delete()
	{
		SetTo(nullValue);
	}

	inline bool IsSet() const
	{
		Checker isHandleSet;
		return isHandleSet(fHandle);
	}

	inline C Get() const
	{
		return fHandle;
	}

	inline C Detach()
	{
		C handle = fHandle;
		fHandle = nullValue;
		return handle;
	}

protected:
	C			fHandle;

private:
	HandleDeleter(const HandleDeleter&);
	HandleDeleter& operator=(const HandleDeleter&);
};

#endif	// _AUTO_DELETER_H
