/*						ARK Project - Adaptive Runtime Kernel

	Module:
		CountingSemAsm.s

	Purpose:
		Atomic counting semaphore management routines for ARM Cortex-M.

	Description:
		A counting semaphore is a shared counter representing the number of available resources or events.
		GetCountingSem() atomically decrements the counter only when it is greater than zero and returns true
		when the resource has been acquired. If the counter is zero, it leaves the counter unchanged and returns
		false. PutCountingSem() atomically increments the counter to release one resource or signal one event.

		On ARM Cortex-M cores these operations are implemented with the LDREX/STREX exclusive-access sequence.
		LDREX reads the current value and marks the addressed word for exclusive access, while STREX attempts
		the write and reports whether another access invalidated the exclusive monitor. If the write fails, the
		operation is retried from the beginning.

		The DMB barrier orders memory accesses around successful acquire/release operations, so protected data is
		accessed consistently with respect to the semaphore operation.

		These primitives do not impose a maximum semaphore value; callers must prevent counter overflow if a
		bounded counting semaphore is required.

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
 	.syntax unified						/* use unified assembler syntax */
	.code 16							/* assemble in Thumb-2  (.thumb" can also be used) */

	//.module	CountingSemAsm
	.text								/* put into linker code section */
	.global GetCountingSem				/* external linkage for my ISR name */
	.thumb_func							/* we are a thumb function */
	.type	GetCountingSem, %function	/* optional: mark it as a function */
	.global PutCountingSem				/* external linkage for my ISR name */
	.thumb_func							/* we are a thumb function */
	.type	PutCountingSem, %function	/* optional: mark it as a function */

GetCountingSem:
	LDREX	r1, [r0]
	CMP		r1, #0				// If the semaphore is already empty
	BEQ		ReturnFalse
	SUB		r1, #1				// If not, decrement temporary copy
	STREX	r2, r1, [r0]		// Attempt Store-Exclusive
	CMP		r2, #0				// Check if Store-Exclusive succeeded
	BNE		GetCountingSem		// If Store-Exclusive failed, retry from start

	MOV		R0, #1
	DMB							// Required before accessing protected resource
	BX		lr
ReturnFalse:
	MOV	R0, #0					// Return false.
	DMB
	BX	lr

PutCountingSem:
	LDREX	r1, [r0]
	ADD		r1, #1				// Increment temporary copy
	STREX	r2, r1, [r0]		// Attempt Store-Exclusive
	CMP		r2, #0				// Check if Store-Exclusive succeeded
	BNE		PutCountingSem		// Store failed - retry immediately
	DMB							// Required before releasing protected resource
	BX		lr

	.END
