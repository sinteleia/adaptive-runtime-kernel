/*						ARK Project - Adaptive Runtime Kernel

	Module:
		LastErr.c

	Purpose:
		RTK per-task last-error access implementation.

	Description:
		This module implements accessors for the local last-error value stored in the current task
		descriptor when LOCAL_LAST_ERROR is enabled. Keeping a dedicated last-error value for each
		task makes concurrent errors explicit and prevents one task from overwriting the diagnostic
		state of another task.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK last-error implementations.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include "LastErr.h"
#include "Sched.h"
 
#if LOCAL_LAST_ERROR

 WORD GetLastError(void){
  return CurrentTaskPtr->LocalLastError;
 }
 
 void SetLastError(WORD LastError){
  CurrentTaskPtr->LocalLastError=LastError;
 }
 
#endif
