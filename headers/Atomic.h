#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <stddef.h>
#include <stdint.h>


static ALWAYS_INLINE void atomic_set(int32_t* value, int32_t newValue)
{
	__atomic_store_n(value, newValue, __ATOMIC_RELEASE);
}

static ALWAYS_INLINE int32_t atomic_get_and_set(int32_t* value, int32_t newValue)
{
	return __atomic_exchange_n(value, newValue, __ATOMIC_SEQ_CST);
}

static ALWAYS_INLINE int32_t atomic_test_and_set(int32_t* value, int32_t newValue, int32_t testAgainst)
{
	__atomic_compare_exchange_n(value, &testAgainst, newValue, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return testAgainst;
}

static ALWAYS_INLINE int32_t atomic_add(int32_t* value, int32_t addValue)
{
	return __atomic_fetch_add(value, addValue, __ATOMIC_SEQ_CST);
}

static ALWAYS_INLINE int32_t atomic_and(int32_t* value, int32_t andValue)
{
	return __atomic_fetch_and(value, andValue, __ATOMIC_SEQ_CST);
}

static ALWAYS_INLINE int32_t atomic_or(int32_t* value, int32_t orValue)
{
	return __atomic_fetch_or(value, orValue, __ATOMIC_SEQ_CST);
}

static ALWAYS_INLINE int32_t atomic_get(int32_t* value)
{
	return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}

static ALWAYS_INLINE void atomic_set64(int64_t* value, int64_t newValue)
{
	__atomic_store_n(value, newValue, __ATOMIC_RELEASE);
}

static ALWAYS_INLINE int64_t atomic_get_and_set64(int64_t* value, int64_t newValue)
{
	return __atomic_exchange_n(value, newValue, __ATOMIC_SEQ_CST);
}

static ALWAYS_INLINE int64_t atomic_test_and_set64(int64_t* value, int64_t newValue, int64_t testAgainst)
{
	__atomic_compare_exchange_n(value, &testAgainst, newValue, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return testAgainst;
}

static ALWAYS_INLINE int64_t atomic_add64(int64_t* value, int64_t addValue)
{
	return __atomic_fetch_add(value, addValue, __ATOMIC_SEQ_CST);
}

static ALWAYS_INLINE int64_t atomic_and64(int64_t* value, int64_t andValue)
{
	return __atomic_fetch_and(value, andValue, __ATOMIC_SEQ_CST);
}

static ALWAYS_INLINE int64_t atomic_or64(int64_t* value, int64_t orValue)
{
	return __atomic_fetch_or(value, orValue, __ATOMIC_SEQ_CST);
}

static ALWAYS_INLINE int64_t atomic_get64(int64_t* value)
{
	return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}


#endif	// _ATOMIC_H_
