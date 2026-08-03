/*						ARK Project - Adaptive Runtime Kernel

	Module:
		MyNew.cpp

	Purpose:
		C++ global allocation operator bindings for the ARK memory manager.

	Description:
		This module redirects global C++ new/delete and new[]/delete[] operations to the ARK memory
		manager by using malloc() and free(). Both non-sized and sized delete operators are provided
		for C++14-compatible toolchains.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Created for ARK 1.0 C++ integration from older RTK/MM allocation support.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include <cstddef>
#include <cstdlib>
#include "mm.h"

// NEW
void* operator new(std::size_t n){ return malloc(n); }
void* operator new[](std::size_t n){ return malloc(n); }

// DELETE (non-sized)
void  operator delete(void* p) noexcept  { free(p); }
void  operator delete[](void* p) noexcept { free(p); }

// DELETE (sized) - C++14+
void  operator delete(void* p, std::size_t) noexcept { free(p); }
void  operator delete[](void* p, std::size_t) noexcept { free(p); }
