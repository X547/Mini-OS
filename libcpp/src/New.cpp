#include <new>
#include <malloc.h>

const std::nothrow_t std::nothrow = std::nothrow_t{};


void *operator new(size_t size)
{
	return malloc(size);
}

void* operator new(size_t size, const std::nothrow_t& tag) noexcept
{
	return malloc(size);
}

void* operator new[](size_t size, const std::nothrow_t& tag) noexcept
{
	return malloc(size);
}

void operator delete(void *obj)
{
	free(obj);
}

void operator delete[](void *obj)
{
	free(obj);
}
