/*						ARK Project - Adaptive Runtime Kernel

	Module:
		LastErr.h

	Purpose:
		Public interface for RTK per-task last-error services.

	Description:
		This header declares accessors for the local last-error value associated with the current task.
		A dedicated last-error value per task makes concurrent errors explicit and avoids using one
		global diagnostic state shared by all tasks.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK last-error headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __LastErr_h
 #define __LastErr_h
 
 #include "Type.h"
 
 #ifdef __cplusplus
  extern "C" {
 #endif

 WORD GetLastError(void);
 void SetLastError(WORD);

 #ifdef __cplusplus
  }
 #endif
 
#endif
