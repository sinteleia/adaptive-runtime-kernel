/*						ARK Project - Adaptive Runtime Kernel

	Module:
		RTK_Error.h

	Purpose:
		Public and internal RTK terminal error handling interface.

	Description:
		This header declares RTK fatal error entry and dispatch services and provides CauseError(),
		which records the debug error code when enabled and enters the unrecoverable error path.
		The dispatch routine may be overridden by the application to implement the project safe state.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK error handling headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef _RTK_ERROR_H_
	#define _RTK_ERROR_H_
	#include "Type.h"

	#ifdef DEBUG
		extern WORD Error;
	#endif

	#ifdef __cplusplus
		extern "C"{
	#endif

	void NonMaskableInt_Handler(void);
	void RTK_UnrecoverableErrorDispatch(WORD Reason);

	#ifdef __cplusplus
		}
	#endif

	static inline void CauseError(WORD Err){
		#ifdef DEBUG
			Error=Err;
		#endif
			RTK_UnrecoverableErrorDispatch(Err);
	}

#endif
