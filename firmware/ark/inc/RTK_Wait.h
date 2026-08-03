/*						ARK Project - Adaptive Runtime Kernel

	Module:
		RTK_Wait.h

	Purpose:
		Public interface for RTK task wait and resume primitives.

	Description:
		This header declares the RTK wait services used by tasks to suspend execution until time,
		timeout, flags, bits, queues, binary semaphores, counting semaphores or explicit resume
		conditions are satisfied. For most wait conditions, the interface provides plain wait,
		check-and-wait, timeout and check-and-wait-with-timeout variants.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK wait primitive headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __Wait_h
	#define __Wait_h

	/* 				RTK wait services principle

		A task can suspend till a specific condition is satisfied.

		For most wait conditions, four variants are available:
		- Wait suspends the current task until a specific condition is satisfied.
		- CheckAndWait checks the condition before suspending the task.
		- WaitTO suspends the task until a specific condition is satisfied or a timeout is reached.
		- CheckAndWaitTO checks the condition before suspending the task for a condition and a timeout.

		Timed out variants return false when the timeout expires and true when the wait condition is satisfied.

		When a timed wait condition is satisfied before its timeout expires, the timer remains active. Any subsequent timed wait 
		with Time parameter equal to zero reuses that timer, allowing one overall timeout to span multiple consecutive wait 
		operations.
	*/

	#include "type.h"
	#include "timer.h"
	#include "sched.h"

	#ifdef __cplusplus
		extern "C" {
	#endif

	void WaitForever(void);			// 	Suspend the current intil it is explicitly resumed.
	bool WaitForeverTO(DWORD Time);	// Suspend the current task until it is explicitly resumed or its timeout expires.
	void ResumeTask(T_TaskDescriptor *TaskToResumeHND);	// Make ready a suspended task.

	void WaitForTime(DWORD Time);

	void WaitForQueEmpty(TQueHeader *Q);
	bool WaitForQueEmptyTO(TQueHeader *Q, DWORD Time);
	void CheckAndWaitForQueEmpty(TQueHeader *Q);
	bool CheckAndWaitForQueEmptyTO(TQueHeader *Q, DWORD Time);
	void WaitForQueGet(TQueHeader *Q);
	void CheckAndWaitForQueGet(TQueHeader *Q);
	bool WaitForQueGetTO(TQueHeader *Q, DWORD Time);
	bool CheckAndWaitForQueGetTO(TQueHeader *Q, DWORD Time);
	void WaitForBynaryLenQuePut(TBinaryLenQueHeader *Q);
	void CheckAndWaitForBynaryLenQuePut(TBinaryLenQueHeader *Q);
	bool WaitForBynaryLenQuePutTO(TBinaryLenQueHeader *Q, DWORD Time);
	bool CheckAndWaitForBynaryLenQuePutTO(TBinaryLenQueHeader *Q, DWORD Time);
	void WaitForFreeLenQuePut(TFreeLenQueHeader *Q);
	void CheckAndWaitForFreeLenQuePut(TFreeLenQueHeader *Q);
	bool WaitForFreeLenQuePutTO(TFreeLenQueHeader *Q, DWORD Time);
	bool CheckAndWaitForFreeLenQuePutTO(TFreeLenQueHeader *Q, DWORD Time);
	void WaitForFlag(Flag *F);
	bool WaitForFlagTO(Flag *F, DWORD Time);
	void CheckAndWaitForFlag(Flag *F);
	bool CheckAndWaitForFlagTO(Flag *F, DWORD Time);
	void WaitForNotFlag(Flag *F);
	bool WaitForNotFlagTO(Flag *F, DWORD Time);
	void CheckAndWaitForNotFlag(Flag *F);
	bool CheckAndWaitForNotFlagTO(Flag *F, DWORD Time);
	void WaitForBit(volatile BYTE *Add, WORD Bit);
	bool WaitForBitTO(volatile BYTE *Add, WORD Bit, DWORD Time);
	void CheckAndWaitForBit(volatile BYTE *Add, WORD Bit);
	bool CheckAndWaitForBitTO(volatile BYTE *Add, WORD Bit, DWORD Time);
	void WaitForNotBit(volatile BYTE *Add, WORD Bit);
	bool WaitForNotBitTO(volatile BYTE *Add, WORD Bit, DWORD Time);
	void CheckAndWaitForNotBit(volatile BYTE *Add, WORD Bit);
	bool CheckAndWaitForNotBitTO(volatile BYTE *Add, WORD Bit, DWORD Time);
	void WaitForWordBit(volatile WORD *Add, WORD Bit);
	bool WaitForWordBitTO(volatile WORD *Add, WORD Bit, DWORD Time);
	void CheckAndWaitForWordBit(volatile WORD *Add, WORD Bit);
	bool CheckAndWaitForWordBitTO(volatile WORD *Add, WORD Bit, DWORD Time);
	void WaitForDWordBit(volatile DWORD *Add, WORD Bit);
	bool WaitForDWordBitTO(volatile DWORD *Add, WORD Bit, DWORD Time);
	void CheckAndWaitForDWordBit(volatile DWORD *Add, WORD Bit);
	bool CheckAndWaitForDWordBitTO(volatile DWORD *Add, WORD Bit, DWORD Time);
	void WaitForWordNotBit(volatile WORD *Add, WORD Bit);
	bool WaitForWordNotBitTO(volatile WORD *Add, WORD Bit, DWORD Time);
	void WaitForDWordNotBit(volatile DWORD *Add, WORD Bit);
	bool WaitForDWordNotBitTO(volatile DWORD *Add, WORD Bit, DWORD Time);
	void CheckAndWaitForWordNotBit(volatile WORD *Add, WORD Bit);
	bool CheckAndWaitForWordNotBitTO(volatile WORD *Add, WORD Bit, DWORD Time);
	void CheckAndWaitForDWordNotBit(volatile DWORD *Add, WORD Bit);
	bool CheckAndWaitForDWordNotBitTO(volatile DWORD *Add, WORD Bit, DWORD Time);
	void WaitForSem(Semaphore *S);
	bool WaitForSemTO(Semaphore *S, DWORD Time);
	void CheckAndWaitForSem(Semaphore *S);
	bool CheckAndWaitForSemTO(Semaphore *S, DWORD Time);
	void WaitForCountingSem(T_CountingSem *S);
	bool WaitForCountingSemTO(T_CountingSem *S, DWORD Time);
	void CheckAndWaitForCountingSem(T_CountingSem *S);
	bool CheckAndWaitForCountingSemTO(T_CountingSem *S, DWORD Time);
	void WaitForAlmenoUnBit(volatile BYTE *Add, BYTE Mask);
	bool WaitForAlmenoUnBitTO(volatile BYTE *Add, BYTE Mask, DWORD Time);
	void CheckAndWaitForAlmenoUnBit(volatile BYTE *Add, BYTE Mask);
	bool CheckAndWaitForAlmenoUnBitTO(volatile BYTE *Add, BYTE Mask, DWORD Time);
	void WaitForNessunBit(volatile BYTE *Add,  BYTE Mask);
	bool WaitForNessunBitTO(volatile BYTE *Add,  BYTE Mask, DWORD Time);
	void CheckAndWaitForNessunBit(volatile BYTE *Add,  BYTE Mask);
	bool CheckAndWaitForNessunBitTO(volatile BYTE *Add,  BYTE Mask, DWORD Time);
	void WaitForAlmenoUnWordBit(volatile WORD *Add, WORD Mask);
	bool WaitForAlmenoUnWordBitTO(volatile WORD *Add, WORD Mask, DWORD Time);
	void CheckAndWaitForAlmenoUnWordBit(volatile WORD *Add, WORD Mask);
	bool CheckAndWaitForAlmenoUnWordBitTO(volatile WORD *Add, WORD Mask, DWORD Time);
	void WaitForNessunWordBit(volatile WORD *Add,  WORD Mask);
	bool WaitForNessunWordBitTO(volatile WORD *Add,  WORD Mask, DWORD Time);
	void CheckAndWaitForNessunWordBit(volatile WORD *Add,  WORD Mask);
	bool CheckAndWaitForNessunWordBitTO(volatile WORD *Add,  WORD Mask, DWORD Time);
	void WaitForAlmenoUnDWordBit(volatile DWORD *Add, DWORD Mask);
	bool WaitForAlmenoUnDWordBitTO(volatile DWORD *Add, DWORD Mask, DWORD Time);
	void CheckAndWaitForAlmenoUnDWordBit(volatile DWORD *Add, DWORD Mask);
	bool CheckAndWaitForAlmenoUnDWordBitTO(volatile DWORD *Add, DWORD Mask, DWORD Time);
	void WaitForNessunDWordBit(volatile DWORD *Add,  DWORD Mask);
	bool WaitForNessunDWordBitTO(volatile DWORD *Add,  DWORD Mask, DWORD Time);
	void CheckAndWaitForNessunDWordBit(volatile DWORD *Add,  DWORD Mask);
	bool CheckAndWaitForNessunDWordBitTO(volatile DWORD *Add,  DWORD Mask, DWORD Time);

	#ifdef __cplusplus
		}
	#endif

#endif
