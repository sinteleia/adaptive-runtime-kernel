/*						ARK Project - Adaptive Runtime Kernel

	Module:
		CountingSem.h

	Purpose:
		Public type and primitive declarations for RTK counting semaphores.

	Description:
		A counting semaphore is a shared counter representing available resources or pending events.
		GetCountingSem() tries to acquire one unit and returns false when no unit is available.
		PutCountingSem() releases one unit by incrementing the counter.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK versions.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef COUNTINGSEM_H_
	#define COUNTINGSEM_H_

	#include "Type.h"

	typedef volatile DWORD T_CountingSem;

	#ifdef __cplusplus
	extern "C" {
		#endif

		bool GetCountingSem(T_CountingSem *Sem);
		void PutCountingSem(T_CountingSem *Sem);

	#ifdef __cplusplus
		}
	#endif

#endif
