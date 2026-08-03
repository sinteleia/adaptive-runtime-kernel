/*						ARK Project - Adaptive Runtime Kernel

	Module:
		timer.h

	Purpose:
		Timer object type and inline timer state helpers.

	Description:
		This header defines the RTK timer object, timer queue status values and inline helpers used to
		initialize a timer and test whether it is elapsed, not armed or still pending in the timer queue.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK timer headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __TIMER_h
	#define __TIMER_h

	/*---------------------------------------------------------------------------------------------------------------------------------

							Management of a timer queue (of type `T_Timer`);

			The timers are managed in a queue sorted by expiration order, so that at each tick it is not necessary to check all of them.
		Instead, starting from the first timer due to expire, timers are checked until one is found that has not yet expired. This type
		of management, by verifying timer expiration at every tick, avoids the possibility of counter overlap and therefore makes it
		possible to determine with certainty whether a timer has expired or not. The trade-off of this technique is the non-deterministic
		tick execution time, since nothing prevents multiple timers from expiring during the same tick, as well as the non-deterministic
		time required to set timers, because during this phase the expiration order of the timer queue must be preserved.
			The advantage of this technique, besides preventing counter overlap problems, is that checking whether a timer has expired
		or not only requires a simple equality comparison between a variable and a constant.
			Timers are organized in a linled list. If the pointer to the next is equal to NULL, the timer is valid and is the last in
		the list, if it is self pointing, the timer is expired.

	----------------------------------------------------- ----------------------------------------------------------------------------*/

	#include "Type.h"

	struct S_Timer{
		DWORD Time;
		struct S_Timer *volatile Next;	// Se Next è uguale NULL il timer è l'ultimo che deve scadere;
										// Se Next è uguale a this, il timer non è in anello, cioè è già scaduto.
	};

	typedef struct S_Timer T_Timer;

	typedef enum{
		TimerQueOk,
		TimerNumberError,
		TimerSequenceError,
		TimerExpiredInQue,
	}T_TimerStatus;

	#ifdef __cplusplus
		extern "C" {
	#endif

	/*
								InitTimer

		Purpose:
			Initialize a timer object and mark it as elapsed or not armed.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Available to RTK code and user application code before a T_Timer object is used by timer services.
		Input:
			T - Pointer to the timer object to initialize.
		Output:
			The timer object pointed to by T is updated.
		Notes:
			A timer with Next pointing to itself is considered elapsed or not currently inserted in the timer queue.
	*/
	static inline void InitTimer(T_Timer *T){
		 T->Next=T;
	}

	/*
		IsTimerPtrElapsed

		Purpose:
			Check whether the timer pointed to by T is elapsed or not armed.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Available to RTK code and user application code that need to query a timer object by pointer.
		Input:
			T - Pointer to the timer object to check.
		Output:
			true when the timer is elapsed or not armed, false otherwise.
		Notes:
			A timer with Next pointing to itself is considered elapsed or not currently inserted in the timer queue.
	*/
	inline bool IsTimerPtrElapsed(T_Timer *T){
		return T==T->Next;
	}

	/*
		IsTimerPtrNotElapsed

		Purpose:
			Check whether the timer pointed to by T is still armed and not elapsed.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Available to RTK code and user application code that need to query a timer object by pointer.
		Input:
			T - Pointer to the timer object to check.
		Output:
			true when the timer is still armed and not elapsed, false otherwise.
		Notes:
			A timer with Next different from itself is considered not elapsed and inserted in one timer queue.
	*/
	inline bool IsTimerPtrNotElapsed(T_Timer *T){
		return T!=T->Next;
	}

	#define IS_TIMER_ELAPSED(X) ((&(X))==(X).Next)
	#define IS_TIMER_NOT_ELAPSED(X) ((&(X))!=(X).Next)

	#ifdef __cplusplus
		}
	#endif

#endif
