/*						ARK Project - Adaptive Runtime Kernel

	Module:
		RTK_Error.c

	Purpose:
		Default RTK terminal error handling implementation.

	Description:
		This module provides the default handlers used when RTK reaches a non-recoverable error path.
		The weak unrecoverable error dispatch routine can be overridden by the application to place the
		system in a safe state. In debug builds, the default handlers break into the debugger before
		stopping execution.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK error handling implementations.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include "RTK_Error.h"

#ifdef DEBUG
	WORD Error;
#endif

/*
	NonMaskableInt_Handler

	Purpose:
		Default NMI trap.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called if the application NMI vector reaches the RTK default handler.
	Input:
		None.
	Output:
		Does not return.
*/
void NonMaskableInt_Handler(void){
	#ifdef DEBUG
		__asm("BKPT #0\n") ; // Break into the debugger
	#endif
	while(1) {
	}
}

/*
	RTK_UnrecoverableErrorDispatch

	Purpose:
		Default terminal handler for unrecoverable RTK runtime errors.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by RTK_UnrecoverableErrorEntry() after interrupts and configurable faults have been masked and the stack has
		been restored to the startup MSP value.
	Input:
		Reason - Unrecoverable error reason code.
	Output:
		Does not return.
	Notes:
		The application may override this weak routine to put the system in a safe state. The override must not return.
*/
__attribute__((weak)) void RTK_UnrecoverableErrorDispatch(WORD Reason){
	(void)Reason;
	#ifdef DEBUG
		__asm("BKPT #0\n") ; // Break into the debugger
	#endif
	while(1) {
	}
}
