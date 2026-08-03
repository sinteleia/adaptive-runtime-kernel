/*						ARK Project - Adaptive Runtime Kernel

	Module:
		SemAsm.s

	Purpose:
		Atomic binary semaphore management routines for ARM Cortex-M.

	Description:
		A binary semaphore represents a resource that can be either free or locked. TestAndSet()
		atomically tests the semaphore and locks it when it is free. It returns true when the lock is
		acquired and false when the semaphore is already locked. Release() marks the semaphore as free.

		On ARM Cortex-M cores TestAndSet() is implemented with the LDREXB/STREXB exclusive-access
		sequence. LDREXB reads the semaphore byte and marks the addressed byte for exclusive access,
		while STREXB attempts to write the locked value and reports whether another access invalidated
		the exclusive monitor. If the write fails, the operation is retried from the beginning.

		The semaphore values are SEM_FREE and SEM_LOCKED as defined in Sem.h. The assembly primitives
		access the semaphore storage with byte operations.

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
	.syntax unified /* use unified assembler syntax */
	.code 16							// assemble in Thumb-2  (.thumb" can also be used)

	//.module	SemAsm
	.text								// put into linker code section 
	.global TestAndSet					// external linkage for my ISR name
	.thumb_func							// we are a thumb function
	.type   TestAndSet, %function		// optional: mark it as a function
	.global Release						// external linkage for my ISR name
	.thumb_func							// we are a thumb function
	.type   Release, %function			// optional: mark it as a function
 
/*				TestAndSet
	Atomically tests and locks a binary semaphore. The semaphore address is passed in R0.
	Returns R0=0 if the semaphore is already locked and R0=1 if the lock is acquired.
	It must return exactly 1 on success because C++ bool true is represented by 1.
*/
TestAndSet:
	MOV	R1,R0			// Use R1 for the semaphore address and free R0
retry:
	LDREXB	R0,[R1]
	CMP		R0,#0		// If it was 0, the semaphore is locked
	BEQ		Ret			// Then return 0
	LDR		R2,=0		// Put the locked value in R2
	STREXB	R0,R2,[R1]	// Try to write 0 to the semaphore
	EORS	R0, R0, #1	// Convert STREXB result to bool success
	BEQ		retry		// Retry if the exclusive write failed
Ret:
	MOV		PC,LR
	
/*				Release
	Releases a previously locked binary semaphore.
*/
Release:
	LDR		R1,=1
	STRB	R1,[R0]
	MOV		PC,LR

	.END
