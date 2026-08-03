/*						ARK Project - Adaptive Runtime Kernel

	Module:
		Sem.h

	Purpose:
		Public type, constants and primitive declarations for RTK binary semaphores.

	Description:
		A binary semaphore represents a resource that can be free or locked.
		TestAndSet() tries to acquire the semaphore and returns false when it is already locked.
		Release() releases a previously acquired semaphore.

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
#ifndef __Semaphore_h
	#define __Semaphore_h

	#include "Type.h"

	#define SEM_FREE 1
	#define SEM_LOCKED 0

	typedef volatile WORD Semaphore;
	typedef volatile bool Flag;

	#ifdef __cplusplus
		extern "C" {
	#endif

	bool TestAndSet(Semaphore *Sem);
	void Release(Semaphore *Sem);

	#ifdef __cplusplus
		}
	#endif

	#define RELEASE_SEM(Sem) Sem=SEM_FREE
	#define RELEASE_SEM_PTR(PSem) (*PSem)=SEM_FREE

#endif
