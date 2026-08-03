/*						ARK Project - Adaptive Runtime Kernel

	Module:
		TimerTic.c

	Purpose:
		RTK timer queue processing driven by the system tick.

	Description:
		This module manages the ordered queue of active RTK timers. It initializes the timer queue,
		arms and disarms timer objects, advances the global timer counter on each system tick and
		expires timers scheduled for the current tick. Timer queue protection is selected by the timer
		configuration options.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK timer tick implementations.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include "TimerTic.h"
#include "General.h"
#include "MyIntrinsics.h"
#include "ErrCode.h"
#include <stddef.h>

#if TIMER_INTERRUPT_PROTECT
	#define TIMER_START_PROTECTION START_PROTECTION
	#define TIMER_END_PROTECTION END_PROTECTION
#else
	#include "Sched.h"
	#define TIMER_START_PROTECTION uint32_t TIMER_SysTicLock=RTK_SysTicLock()
	#define TIMER_END_PROTECTION RTK_Unlock(TIMER_SysTicLock)
#endif

volatile DWORD TimerCtr;
T_Timer *FirstToTic=NULL;

#if TIMER_NUMBER_CHECK
	unsigned int NumberOfActiveTimers=0;
#endif


/*
 									InitTimerTic

	Purpose:
		Initialize the timer tick queue.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called during RTK initialization before timer services are used.
	Input:
		None.
	Output:
		FirstToTic is cleared.
 */
inline void InitTimerTic(void){
	FirstToTic=NULL;
	#if TIMER_NUMBER_CHECK
		NumberOfActiveTimers=0;
	#endif
}

/*
	DisarmaTimer

	Purpose:
		Remove the specified timer from the pending timer queue and mark it as elapsed.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Available to RTK code and user application code when an armed timer must be cancelled.
	Input:
		TimerDaEliminare - Pointer to the timer object to remove from the queue.
	Output:
		The timer is marked as elapsed when it is found in the pending timer queue.
	Notes:
		The timer queue is accessed inside a protected section with interrupts disabled.
*/
void DisarmaTimer(T_Timer *TimerDaEliminare){
	T_Timer *CntTested;
	TIMER_START_PROTECTION;
	if(TimerDaEliminare!=TimerDaEliminare->Next){
		#if TIMER_NUMBER_CHECK
			NumberOfActiveTimers--;
		#endif
		if(FirstToTic){
			if(FirstToTic==TimerDaEliminare){
				FirstToTic=FirstToTic->Next;
				TimerDaEliminare->Next=TimerDaEliminare;
			}
			else{
				#if TIMERS_GUARD && TIMER_NUMBER_CHECK
					int n=NumberOfActiveTimers;
				#endif
				CntTested=FirstToTic;
				do{
					if(CntTested->Next==TimerDaEliminare){
						CntTested->Next=TimerDaEliminare->Next;
						TimerDaEliminare->Next=TimerDaEliminare;
						break;
					}
					#if TIMERS_GUARD && TIMER_NUMBER_CHECK
						if(--n<=0) CauseError(TIMER_LIST_GUARD_ERROR);
					#endif
				}while((CntTested=CntTested->Next)!=NULL);
			}
		}
	}
	TIMER_END_PROTECTION;
}

/*
	TimerTic

	Purpose:
		Advance the system timer counter and expire all timers scheduled for the current tick.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called once per system tick by the RTK tick handling path.
	Input:
		None.
	Output:
		TimerCtr is incremented, expired timers are removed from the queue, and their Next pointer is set to themselves.
	Notes:
		The queue is ordered by expiration time, so scanning stops at the first timer that is not due on the current tick.
*/
void TimerTic(){
	T_Timer *Tmp;
	TimerCtr++;
	while(FirstToTic){
		if(FirstToTic->Time!=TimerCtr)
			break;
		#if TIMER_NUMBER_CHECK
			NumberOfActiveTimers--;
		#endif
		Tmp=FirstToTic;
		FirstToTic=FirstToTic->Next;
		Tmp->Next=Tmp;
	}
}

/*
	SetTimer

	Purpose:
		Arm a timer with a relative delay and insert it in the pending timer queue.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Available to RTK code and user application code when a timer must be started or rearmed.
	Input:
		T - Pointer to the timer object to arm.
		Value - Relative delay, in timer ticks, before the timer expires.
	Output:
		When Value is not zero, the timer is removed from any previous queue position and inserted at the new expiration point.
	Notes:
		The timer queue is accessed inside a protected section with interrupts disabled. If Value is zero, no operation is
		performed and the current timer state is preserved.
*/
void SetTimer(T_Timer *T, DWORD Value){
	T_Timer *NextToCheck;
	if(Value){
		DisarmaTimer(T);
		TIMER_START_PROTECTION;
		#if TIMER_NUMBER_CHECK
			NumberOfActiveTimers++;
		#endif
		T->Time=Value+TimerCtr;
		if(FirstToTic){
			// Build a virtual previous timer whose Next field aliases FirstToTic.
			NextToCheck=(T_Timer *)((char *)&FirstToTic - offsetof(T_Timer, Next));
			while(NextToCheck->Next){
					if(NextToCheck->Next->Time-TimerCtr>=Value)
						break;
					NextToCheck=NextToCheck->Next;
			}
			T->Next=NextToCheck->Next;
			NextToCheck->Next=T;
		}
		else{
			FirstToTic=T;
			T->Next=NULL;
		}
		TIMER_END_PROTECTION;
	}
}

/*
	TimerTicQuantoManca

	Purpose:
		Return the remaining tick count before the specified timer expires.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Available to RTK code and user application code that need to inspect an armed timer.
	Input:
		T - Pointer to the timer object to inspect.
	Output:
		Remaining ticks before expiration, or zero when the timer is elapsed or not armed.
*/
DWORD TimerTicQuantoManca(T_Timer *T){
	DWORD TmpRetValue=T->Time-TimerCtr;
	return (T==T->Next)? 0U: TmpRetValue;
}

/*
							CheckTimerStatus

	Purpose:
		Check the timer que for popssible incongruence.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Available to RTK code and user application code that need to check for timer corruptions.
	Input:
		---
	Output:
		T_TimerStatus enum indicating the current timer que status.
	Note:
		Checked error:
			- Number of tic in the que differs from the registred number;
			- An expired (Next=this) timer is in the que;
			- Sequence error (for every n>0 Timer[n]-TimerCtr < Timer[n-1]-TimerCtr))
*/
T_TimerStatus CheckTimerStatus(){
	#if TIMER_NUMBER_CHECK
		unsigned int N=0;
	#endif
	T_TimerStatus Esito=TimerQueOk;
	DWORD PrevTime=0;
	TIMER_START_PROTECTION;
	T_Timer *NextToCheck=FirstToTic;
	while(NextToCheck){
		DWORD tmp=NextToCheck->Time-TimerCtr;
		if(tmp<PrevTime){
			Esito=TimerSequenceError;
			break;
		}
		PrevTime=tmp;
		#if TIMER_NUMBER_CHECK
			N++;
		#endif
		if(NextToCheck==NextToCheck->Next){
			Esito=TimerExpiredInQue;
			break;
		}
		NextToCheck=NextToCheck->Next;
	}
	#if TIMER_NUMBER_CHECK
		if((Esito==TimerQueOk)&&(N!=NumberOfActiveTimers)) Esito=TimerNumberError;
	#endif
	TIMER_END_PROTECTION;
	return Esito;
}



