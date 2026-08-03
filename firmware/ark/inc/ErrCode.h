/*						ARK Project - Adaptive Runtime Kernel

	Module:
		ErrCode.h

	Purpose:
		RTK and ARK diagnostic and fatal error code definitions.

	Description:
		This header defines the numeric error codes used by RTK and ARK diagnostics, last-error
		reporting and unrecoverable error paths. USER_ERROR_CODES marks the start of the user-defined
		error code range.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK error code headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __Error_Codes_h
	#define __Error_Codes_h

	#ifndef __ASSEMBLER__
		#include "RTK_Error.h"
	#endif

	// Generated when the first heap block is missing, for example because memory was corrupted or malloc was never initialized.
	#define MALLOC_ERROR_NO_FIRST_BLOCK 1
	// Generated when an allocation request is larger than the maximum available block.
	#define MALLOC_ERROR_NO_SPACE 2
	// Generated when more tic routines than the configured maximum are attached.
	#define AGGANCIA_TIC_ERROR_NO_SPACE 3
	// Generated when more scheduler routines than the configured maximum are attached.
	#define AGGANCIA_SCHED_ERROR_NO_SPACE 4
	// Generated when an attempt is made to free an already free handler.
	#define ERROR_FREE_HANDLER 6
	// Generated when an operation is attempted on a handler that is not valid for that operation. Detaching an INVALID_HANDLER
	// is a valid operation and does not generate an error.
	#define ERROR_INVALID_HANDLER 7
	// Generated when a non-expired timer is not found in the pending timer list.
	#define TIMER_ERROR_NOT_FOUND_IN_TIMER_LIST 8
	// Generated when the active timer count becomes negative.
	#define NUMBER_OF_TIMER_TIC_NEGATIVO 9
	// Generated when timer list scanning finds more timers than expected.
	#define TIMER_LIST_GUARD_ERROR 10
	// Generated when memory cannot be allocated for the configured number of scheduler objects.
	#define SCHED_OBJECT_MEMORY_NOT_FOUND 11
	// Generated when an attempt is made to release an already free disk-cache buffer.
	#define DISK_CACHE_FREE_OF_A_FREE_BUFFER 12
	// Generated when the stack guard check detects a task stack overflow.
	#define RTK_FATAL_STACK_GUARD_ERROR 100
	// Block of errors generated when heap diagnostics detect unrecoverable corruption.
	#define RTK_FATAL_HEAP_CORRUPTION 100
	// Generated when the guard word of an allocated heap block is corrupted.
	#define RTK_FATAL_HEAP_GUARD_ERROR 110

	#define USER_ERROR_CODES 1000

#endif
