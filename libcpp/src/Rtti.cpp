#include <cxxabi.h>
#include <stdlib.h>
#include "Graphics.h"
#include "Modules.h"


extern "C" void __cxa_pure_virtual()
{abort();}

void  __cxxabiv1::__cxa_bad_typeid() __attribute__((__noreturn__))
{abort();}


//#pragma mark type_info
std::type_info::~type_info() {abort();}
bool std::type_info::__is_pointer_p() const {abort(); return false;}
bool std::type_info::__is_function_p() const {abort(); return false;}
bool std::type_info::__do_upcast(const __cxxabiv1::__class_type_info* __dst_type, void**__obj_ptr) const {abort(); return false;}
bool std::type_info::__do_catch(const std::type_info* __thr_type, void** __thr_obj, unsigned __outer) const {abort(); return false;}

//#pragma mark __class_type_info
__cxxabiv1::__class_type_info::~__class_type_info() {abort();}
bool __cxxabiv1::__class_type_info::__do_upcast(const __cxxabiv1::__class_type_info* __dst_type, void**__obj_ptr) const {abort(); return false;}
bool __cxxabiv1::__class_type_info::__do_upcast(const __cxxabiv1::__class_type_info* __dst, const void* __obj, __upcast_result& __restrict __result) const {abort(); return false;}
bool __cxxabiv1::__class_type_info::__do_catch(const std::type_info* __thr_type, void** __thr_obj, unsigned __outer) const {abort(); return false;}
bool __cxxabiv1::__class_type_info::__do_dyncast(ptrdiff_t __src2dst, __sub_kind __access_path, const __class_type_info* __dst_type, const void* __obj_ptr, const __class_type_info* __src_type, const void* __src_ptr, __dyncast_result& __result) const {abort(); return false;}
__cxxabiv1::__class_type_info::__sub_kind __cxxabiv1::__class_type_info::__do_find_public_src(ptrdiff_t __src2dst, const void* __obj_ptr, const __class_type_info* __src_type, const void* __src_ptr) const {abort(); return __unknown;}

//#pragma mark __si_class_type_info
__cxxabiv1::__si_class_type_info::~__si_class_type_info() {abort();}
bool __cxxabiv1::__si_class_type_info::__do_upcast(const __cxxabiv1::__class_type_info* __dst, const void* __obj, __upcast_result& __restrict __result) const {abort(); return false;}
bool __cxxabiv1::__si_class_type_info::__do_dyncast(ptrdiff_t __src2dst, __sub_kind __access_path, const __class_type_info* __dst_type, const void* __obj_ptr, const __class_type_info* __src_type, const void* __src_ptr, __dyncast_result& __result) const {abort(); return false;}
__cxxabiv1::__class_type_info::__sub_kind __cxxabiv1::__si_class_type_info::__do_find_public_src(ptrdiff_t __src2dst, const void* __obj_ptr, const __class_type_info* __src_type, const void* __src_ptr) const {abort(); return __unknown;}

//#pragma mark __vmi_class_type_info
__cxxabiv1::__vmi_class_type_info::~__vmi_class_type_info() {abort();}
bool __cxxabiv1::__vmi_class_type_info::__do_upcast(const __cxxabiv1::__class_type_info* __dst, const void* __obj, __upcast_result& __restrict __result) const {abort(); return false;}
bool __cxxabiv1::__vmi_class_type_info::__do_dyncast(ptrdiff_t __src2dst, __sub_kind __access_path, const __class_type_info* __dst_type, const void* __obj_ptr, const __class_type_info* __src_type, const void* __src_ptr, __dyncast_result& __result) const {abort(); return false;}
__cxxabiv1::__class_type_info::__sub_kind __cxxabiv1::__vmi_class_type_info::__do_find_public_src(ptrdiff_t __src2dst, const void* __obj_ptr, const __class_type_info* __src_type, const void* __src_ptr) const {abort(); return __unknown;}


void WriteType(const char *name, const void *__src_type)
{
	WriteString(name); WriteString(": "); Modules::WritePC((size_t)__src_type); WriteLn();
	WriteString(name); WriteString("->vtable: "); Modules::WritePC(*(size_t*)__src_type); WriteLn();
	WriteString(name); WriteString("->vtable->typeInfo: "); Modules::WritePC(*((*(size_t**)__src_type) - 1)); WriteLn();
}

//#pragma mark -
extern "C" void* __dynamic_cast(
	const void* __src_ptr,
	const __cxxabiv1::__class_type_info* __src_type,
	const __cxxabiv1::__class_type_info* __dst_type,
	ptrdiff_t __src2dst
)
{
/*
	WriteString("__dynamic_cast(0x");
	WriteHex((size_t)__src_ptr, 8); WriteString(", 0x");
	WriteHex((size_t)__src_type, 8); WriteString("("); WriteString(__src_type->name()); WriteString("), 0x");
	WriteHex((size_t)__dst_type, 8); WriteString("("); WriteString(__dst_type->name()); WriteString("))"); WriteLn();
	WriteString("0x"); WriteHex(*(size_t*)__src_type, 8); WriteLn();
	WriteString("0x"); WriteHex(*((*(size_t**)__src_type) - 1), 8); WriteLn();
	WriteString((*((*(__cxxabiv1::__class_type_info***)__src_type) - 1))->name()); WriteLn();
	WriteType("__src_ptr", __src_ptr);
	WriteType("__src_type", __src_type);
	WriteType("__dst_type", __dst_type);
	WriteType("typeid(*__src_type)", &typeid(*__src_type));
	WriteType("typeid(__cxxabiv1::__vmi_class_type_info)", &typeid(__cxxabiv1::__vmi_class_type_info));
*/	
	// WriteString("Base types:"); WriteLn();
	const __cxxabiv1::__class_type_info *type = (*((*(__cxxabiv1::__class_type_info***)__src_ptr) - 1));
	while (type != NULL) {
		// WriteString(type->name()); WriteLn();
		if (type == __dst_type) return (void*)__src_ptr;
		if (typeid(*type) == typeid(__cxxabiv1::__si_class_type_info))
			type = ((__cxxabiv1::__si_class_type_info*)type)->__base_type;
		else if (typeid(*type) == typeid(__cxxabiv1::__class_type_info))
			type = NULL;
		else
			abort();
	}
	return NULL;
}

/*
std::type_info
	const char *__name;

__class_type_info: std::type_info

__si_class_type_info: __class_type_info
	const __class_type_info* __base_type;

__vmi_class_type_info: __class_type_info
	unsigned int __flags;  // Details about the class hierarchy.
	unsigned int __base_count;  // Number of direct bases.


Views::Message
Views::PointerMsg, Views::Message
*/
