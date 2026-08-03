/*						ARK Project - Adaptive Runtime Kernel

	Module:
		RTK_Wait.c

	Purpose:
		RTK task wait and resume primitive implementation.

	Description:
		This module implements the public wait services used by RTK tasks to suspend execution until
		time, timeout, flags, bits, queues, binary semaphores, counting semaphores or explicit resume
		conditions are satisfied. The routines update the current task wait state, configure task
		timers when needed and request scheduling through the RTK scheduler path.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK wait primitive implementations.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include "RTK_Wait.h"
#include "TimerTic.h"
#include "Sem.h"
#include "MyIntrinsics.h"

/*					ResumeTask

		Purpose:
			Makes a task ready again when it is waiting in the unconditional wait state.

		Author:
			Paolo Rozzi

		Reviewer:
			---

		Context:
			This is a public RTK wait function. It checks only the WaitingFor bitfield, so it applies both to
			WaitingForever and WaitingForeverTO. It does not request scheduling directly; the caller is responsible for
			requesting scheduling if immediate execution of the resumed task is required.

		Input:
			TaskToResumeHND: pointer to the task descriptor to resume.

		Output:
			None. If the task is waiting forever, TaskStatus is cleared and the task becomes ready.
*/
void ResumeTask(T_TaskDescriptor *TaskToResumeHND){
	if(TaskToResumeHND->TaskStatus.AsBit.WaitingFor==WaitingForever)
		TaskToResumeHND->TaskStatus.AsByte=WaitingForNone;
}

/*					WaitForever
	Purpose:
		Suspends the current task indefinitely.
		The current task becomes ready again only when another task or an ISR resumes it through ResumeTask().
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK wait function and must be called from task context. It always sets the current task wait
		state and requests scheduling immediately after the state has been stored.
	Input:
		None.
	Output:
		None. The current task is marked as WaitingForever and PendSV is requested.
*/
void WaitForever(){
	#if CALLER_ADDRESS
		CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
	#endif
	CurrentTaskPtr->TaskStatus.AsByte=WaitingForever;
	SCHEDULE;
}

/*					WaitForeverTO
	Purpose:
		Suspends the current task until ResumeTask() is called or the task timeout expires.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK wait function and must be called from task context. If Time is not zero, a new task timer
		is started. If Time is zero, the previously configured task timer is kept, allowing one timeout to cover
		multiple wait operations.
	Input:
		Time: timeout in system ticks, or zero to keep the currently configured task timer.
	Output:
		true if the task has been resumed explicitly.
		false if the wait ended because the timeout expired.
*/
bool WaitForeverTO(DWORD Time){
	#if CALLER_ADDRESS
		CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
	#endif
	if(Time)
		SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
	CurrentTaskPtr->TaskStatus.AsByte=WaitingForeverTO;
	SCHEDULE;
	__disable_irq();
	if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
		__enable_irq();
		return false;
	}
	else{
		__enable_irq();
		return true;
	}
}

/*					WaitForTime
	Purpose:
		Suspends the current task until its task timer expires.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK wait function and must be called from task context. If Time is not zero, a new task timer
		is started. If Time is zero, the previously configured task timer is used. The timer condition is evaluated by
		the scheduler.
	Input:
		Time: wait time in system ticks, or zero to use the currently configured task timer.
	Output:
		None. The current task is marked as WaitingForTime and PendSV is requested.
*/
void WaitForTime(DWORD Time){
	#if CALLER_ADDRESS
		CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
	#endif
	if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
	CurrentTaskPtr->TaskStatus.AsByte=WaitingForTime;
	SCHEDULE;
}

#if WAIT_FOR_QUE_EMPTY

	/*					WaitForQueEmpty
		Purpose:
			Suspends the current task until the queue becomes empty.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It does not check the queue state
			before suspending the task; the wait condition is evaluated by the scheduler.
		Input:
			Q: pointer to the queue header to wait on.
		Output:
			None. The current task waits for WaitingForQueEmpty and PendSV is requested.
*/
	void WaitForQueEmpty(TQueHeader *Q){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.Q=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForQueEmpty;
		SCHEDULE;
	}

	/*					CheckAndWaitForQueEmpty
		Purpose:
			Suspends the current task until the queue becomes empty, only if the queue is not already empty.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It checks the queue state before
			suspending the task; if the queue is already empty, it returns immediately without requesting scheduling.
		Input:
			Q: pointer to the queue header to wait on.
		Output:
			None. If the queue is not already empty, the current task waits for WaitingForQueEmpty and PendSV is requested.
	*/
	void CheckAndWaitForQueEmpty(TQueHeader *Q){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if IS_QUE_PTR_EMPTY(Q) return;
		CurrentTaskPtr->ObjectToWait.Q=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForQueEmpty;
		SCHEDULE;
	}

	/*					WaitForQueEmptyTO
		Purpose:
			Suspends the current task until the queue becomes empty or the task timeout expires.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It does not check the queue state
			before suspending the task. If Time is not zero, a new task timer is started; if Time is zero, the previously
			configured task timer is kept.
		Input:
			Q: pointer to the queue header to wait on.
			Time: timeout in system ticks, or zero to keep the currently configured task timer.
		Output:
			true if the queue became empty before the timeout expired.
			false if the wait ended because the timeout expired.
	*/
	bool WaitForQueEmptyTO(TQueHeader *Q, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.Q=Q;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForQueEmptyTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForQueEmptyTO
		Purpose:
			Suspends the current task until the queue becomes empty or the task timeout expires, only if the queue is
			not already empty.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It checks the queue state before
			suspending the task; if the queue is already empty, it returns true without requesting scheduling. If Time is
			not zero, a new task timer is started; if Time is zero, the previously configured task timer is kept.
		Input:
			Q: pointer to the queue header to wait on.
			Time: timeout in system ticks, or zero to keep the currently configured task timer.
		Output:
			true if the queue is already empty or became empty before the timeout expired.
			false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForQueEmptyTO(TQueHeader *Q, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if IS_QUE_PTR_EMPTY(Q) return true;
		CurrentTaskPtr->ObjectToWait.Q=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForQueEmptyTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_QUE_GET

	/*					WaitForQueGet
		Purpose:
			Suspends the current task until the queue contains at least one element.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It does not check the queue state
			before suspending the task; the wait condition is evaluated by the scheduler.
		Input:
			Q: pointer to the queue header to wait on.
		Output:
			None. The current task waits for WaitingForQueGet and PendSV is requested.
	*/
	void WaitForQueGet(TQueHeader *Q){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.Q=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForQueGet;
		SCHEDULE;
	}

	/*					CheckAndWaitForQueGet
		Purpose:
			Suspends the current task until the queue contains at least one element, only if the queue is currently empty.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It checks the queue state before
			suspending the task; if the queue already contains data, it returns immediately without requesting scheduling.
		Input:
			Q: pointer to the queue header to wait on.
		Output:
			None. If the queue is empty, the current task waits for WaitingForQueGet and PendSV is requested.
	*/
	void CheckAndWaitForQueGet(TQueHeader *Q){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if IS_QUE_PTR_NOT_EMPTY(Q) return;
		CurrentTaskPtr->ObjectToWait.Q=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForQueGet;
		SCHEDULE;
	}

	/*					WaitForQueGetTO
		Purpose:
			Suspends the current task until the queue contains at least one element or the task timeout expires.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It does not check the queue state
			before suspending the task. If Time is not zero, a new task timer is started; if Time is zero, the previously
			configured task timer is kept.
		Input:
			Q: pointer to the queue header to wait on.
			Time: timeout in system ticks, or zero to keep the currently configured task timer.
		Output:
			true if the queue received data before the timeout expired.
			false if the wait ended because the timeout expired.
	*/
	bool WaitForQueGetTO(TQueHeader *Q, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.Q=Q;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForQueGetTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForQueGetTO
		Purpose:
			Suspends the current task until the queue contains at least one element or the task timeout expires, only if
			the queue is currently empty.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It checks the queue state before
			suspending the task; if the queue already contains data, it returns true without requesting scheduling. If
			Time is not zero, a new task timer is started; if Time is zero, the previously configured task timer is kept.
		Input:
			Q: pointer to the queue header to wait on.
			Time: timeout in system ticks, or zero to keep the currently configured task timer.
		Output:
			true if the queue already contains data or received data before the timeout expired.
			false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForQueGetTO(TQueHeader *Q, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if IS_QUE_PTR_NOT_EMPTY(Q) return true;
		CurrentTaskPtr->ObjectToWait.Q=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForQueGetTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_BYNARY_LEN_QUE_PUT

	/*					WaitForBynaryLenQuePut
		Purpose:
			Suspends the current task until a binary length queue has space for one element.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It applies to power-of-two length
			queues represented by TBinaryLenQueHeader. The historical API name Bynary is kept as an RTK symbol.
		Input:
			Q: pointer to the binary length queue header to wait on.
		Output:
			None. The current task waits for WaitingForBynaryLenQuePut and PendSV is requested.
	*/
	void WaitForBynaryLenQuePut(TBinaryLenQueHeader *Q){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.BQ=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBynaryLenQuePut;
		SCHEDULE;
	}

	/*					CheckAndWaitForBynaryLenQuePut
		Purpose:
			Suspends the current task until a binary length queue has space, only if the queue is currently full.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It checks the queue state before
			suspending the task; if space is already available, it returns immediately without requesting scheduling.
			The historical API name Bynary is kept as an RTK symbol.
		Input:
			Q: pointer to the binary length queue header to wait on.
		Output:
			None. If the queue is full, the current task waits for WaitingForBynaryLenQuePut and PendSV is requested.
	*/
	void CheckAndWaitForBynaryLenQuePut(TBinaryLenQueHeader *Q){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if IS_BYNARY_LEN_QUE_PTR_NOT_FULL(Q) return;
		CurrentTaskPtr->ObjectToWait.BQ=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBynaryLenQuePut;
		SCHEDULE;
	}

	/*					WaitForBynaryLenQuePutTO
		Purpose:
			Suspends the current task until a binary length queue has space or the task timeout expires.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It does not check the queue state
			before suspending the task. If Time is not zero, a new task timer is started; if Time is zero, the previously
			configured task timer is kept. The historical API name Bynary is kept as an RTK symbol.
		Input:
			Q: pointer to the binary length queue header to wait on.
			Time: timeout in system ticks, or zero to keep the currently configured task timer.
		Output:
			true if queue space became available before the timeout expired.
			false if the wait ended because the timeout expired.
	*/
	bool WaitForBynaryLenQuePutTO(TBinaryLenQueHeader *Q, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.BQ=Q;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBynaryLenQuePutTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForBynaryLenQuePutTO
		Purpose:
			Suspends the current task until a binary length queue has space or the task timeout expires, only if the queue is 
			currently full.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It checks the queue state before suspending 
			the task; if space is already available, it returns true without requesting scheduling. If Time is not zero, a new task 
			timer is started; if Time is zero, the previously configured task timer is kept. The historical API name Bynary is kept 
			as an RTK symbol.
		Input:
			Q: pointer to the binary length queue header to wait on.
			Time: timeout in system ticks, or zero to keep the currently configured task timer.
		Output:
			true if queue space is already available or became available before the timeout expired.
			false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForBynaryLenQuePutTO(TBinaryLenQueHeader *Q, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if IS_BYNARY_LEN_QUE_PTR_NOT_FULL(Q) return true;
		CurrentTaskPtr->ObjectToWait.BQ=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBynaryLenQuePutTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_FREE_LEN_QUE_PUT

	/*					WaitForFreeLenQuePut
		Purpose:
			Suspends the current task until a free length queue has space for one element.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It applies to exact/free length
			queues represented by TFreeLenQueHeader and should be enabled only when the validated configuration needs it.
		Input:
			Q: pointer to the free length queue header to wait on.
		Output:
			None. The current task waits for WaitingForFreeLenQuePut and PendSV is requested.
	*/
	void WaitForFreeLenQuePut(TFreeLenQueHeader *Q){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.FQ=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForFreeLenQuePut;
		SCHEDULE;
	}

	/*					CheckAndWaitForFreeLenQuePut
		Purpose:
		Suspends the current task until a free length queue has space, only if the queue is currently full.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It checks the queue state before
			suspending the task; if space is already available, it returns immediately without requesting scheduling.
			It should be enabled only when the validated configuration needs exact/free length queues.
		Input:
			Q: pointer to the free length queue header to wait on.
		Output:
			None. If the queue is full, the current task waits for WaitingForFreeLenQuePut and PendSV is requested.
	*/
	void CheckAndWaitForFreeLenQuePut(TFreeLenQueHeader *Q){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if IS_FREE_LEN_QUE_PTR_NOT_FULL(Q) return;
		CurrentTaskPtr->ObjectToWait.FQ=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForFreeLenQuePut;
		SCHEDULE;
	}

	/*					WaitForFreeLenQuePutTO
		Purpose:
			Suspends the current task until a free length queue has space or the task timeout expires.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			This is a public RTK wait function and must be called from task context. It does not check the queue state before 
			suspending the task. If Time is not zero, a new task timer is started; if Time is zero, the previously configured task 
			timer is kept. It should be enabled only when the validated configuration needs exact/free length queues.
		Input:
			Q: pointer to the free length queue header to wait on.
			Time: timeout in system ticks, or zero to keep the currently configured task timer.
		Output:
			true if queue space became available before the timeout expired.
			false if the wait ended because the timeout expired.
	*/
	bool WaitForFreeLenQuePutTO(TFreeLenQueHeader *Q, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.FQ=Q;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForFreeLenQuePutTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForFreeLenQuePutTO

			Purpose:
				Suspends the current task until a free length queue has space or the task timeout expires, only if the queue
				is currently full.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the queue state before
				suspending the task; if space is already available, it returns true without requesting scheduling. If Time is
				not zero, a new task timer is started; if Time is zero, the previously configured task timer is kept.
				It should be enabled only when the validated configuration needs exact/free length queues.

			Input:
				Q: pointer to the free length queue header to wait on.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if queue space is already available or became available before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForFreeLenQuePutTO(TFreeLenQueHeader *Q, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
 		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if IS_FREE_LEN_QUE_PTR_NOT_FULL(Q) return true;
		CurrentTaskPtr->ObjectToWait.FQ=Q;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForFreeLenQuePutTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_FLAG

	/*					WaitForFlag

			Purpose:
				Suspends the current task until the flag becomes true.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It does not check the flag before
				suspending the task; the wait condition is evaluated by the scheduler.

			Input:
				F: pointer to the flag to wait on.

			Output:
				None. The current task waits for WaitingForFlag and PendSV is requested.
	*/
	void WaitForFlag(Flag *F){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.F=F;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForFlag;
		SCHEDULE;
	}

	/*					WaitForFlagTO

			Purpose:
				Suspends the current task until the flag becomes true or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It does not check the flag before
				suspending the task. If Time is not zero, a new task timer is started; if Time is zero, the previously
				configured task timer is kept.

			Input:
				F: pointer to the flag to wait on.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the flag became true before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForFlagTO(Flag *F, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.F=F;
		if(Time)
			SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForFlagTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForFlag

			Purpose:
				Suspends the current task until the flag becomes true, only if the flag is currently false.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the flag before suspending
				the task; if the flag is already true, it returns immediately without requesting scheduling.

			Input:
				F: pointer to the flag to wait on.

			Output:
				None. If the flag is false, the current task waits for WaitingForFlag and PendSV is requested.
	*/
	void CheckAndWaitForFlag(Flag *F){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*F)
			return;
		CurrentTaskPtr->ObjectToWait.F=F;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForFlag;
		SCHEDULE;
	}

	/*					CheckAndWaitForFlagTO

			Purpose:
				Suspends the current task until the flag becomes true or the task timeout expires, only if the flag is
				currently false.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the flag before suspending
				the task; if the flag is already true, it returns true without requesting scheduling. If Time is not zero, a
				new task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				F: pointer to the flag to wait on.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the flag is already true or became true before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForFlagTO(Flag *F, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if(*F) return true;
		CurrentTaskPtr->ObjectToWait.F=F;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForFlagTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}
#endif

#if WAIT_FOR_NOT_FLAG
	/*					WaitForNotFlag

			Purpose:
				Suspends the current task until the flag becomes false.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It does not check the flag before
				suspending the task; the wait condition is evaluated by the scheduler.

			Input:
				F: pointer to the flag to wait on.

			Output:
				None. The current task waits for WaitingForNotFlag and PendSV is requested.
	*/
	void WaitForNotFlag(Flag *F){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.F=F;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotFlag;
		SCHEDULE;
	}

	/*					WaitForNotFlagTO

			Purpose:
				Suspends the current task until the flag becomes false or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It does not check the flag before
				suspending the task. If Time is not zero, a new task timer is started; if Time is zero, the previously
				configured task timer is kept.

			Input:
				F: pointer to the flag to wait on.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the flag became false before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForNotFlagTO(Flag *F, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.F=F;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotFlagTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForNotFlag

			Purpose:
				Suspends the current task until the flag becomes false, only if the flag is currently true.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the flag before suspending
				the task; if the flag is already false, it returns immediately without requesting scheduling.

			Input:
				F: pointer to the flag to wait on.

			Output:
				None. If the flag is true, the current task waits for WaitingForNotFlag and PendSV is requested.
	*/
	void CheckAndWaitForNotFlag(Flag *F){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*F==0) return;
		CurrentTaskPtr->ObjectToWait.F=F;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotFlag;
		SCHEDULE;
	}

	/*					CheckAndWaitForNotFlagTO

			Purpose:
				Suspends the current task until the flag becomes false or the task timeout expires, only if the flag is
				currently true.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the flag before suspending
				the task; if the flag is already false, it returns true without requesting scheduling. If Time is not zero, a
				new task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				F: pointer to the flag to wait on.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the flag is already false or became false before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForNotFlagTO(Flag *F, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if(*F==0) return true;
		CurrentTaskPtr->ObjectToWait.F=F;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotFlagTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}
#endif

#if WAIT_FOR_BIT
	/*					WaitForBit

			Purpose:
				Suspends the current task until one bit in a BYTE becomes set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the BYTE object to test.
				Bit: bit index to wait for, converted to a BYTE mask using (1 << Bit).

			Output:
				None. The current task waits for WaitingForBit and PendSV is requested.
	*/
	void WaitForBit(volatile BYTE *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBit;
		SCHEDULE;
	}

	/*					WaitForBitTO

			Purpose:
				Suspends the current task until one bit in a BYTE becomes set or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the BYTE object to test.
				Bit: bit index to wait for, converted to a BYTE mask using (1 << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForBitTO(volatile BYTE *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(1<<Bit);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForBit

			Purpose:
				Suspends the current task until one bit in a BYTE becomes set, only if the bit is currently clear.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already set, it returns immediately without requesting scheduling.

			Input:
				Add: pointer to the BYTE object to test.
				Bit: bit index to wait for, converted to a BYTE mask using (1 << Bit).

			Output:
				None. If the bit is clear, the current task waits for WaitingForBit and PendSV is requested.
	*/
	void CheckAndWaitForBit(volatile BYTE *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*Add&(1<<Bit)) return;
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForBitTO

			Purpose:
				Suspends the current task until one bit in a BYTE becomes set or the task timeout expires, only if the bit is
				currently clear.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already set, it returns true without requesting scheduling. If Time is not zero, a new
				task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the BYTE object to test.
				Bit: bit index to wait for, converted to a BYTE mask using (1 << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit is already set or became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForBitTO(volatile BYTE *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if(*Add&(1<<Bit)) return true;
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}
#endif

#if WAIT_FOR_NOT_BIT
	/*					WaitForNotBit

			Purpose:
				Suspends the current task until one bit in a BYTE becomes clear.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the BYTE object to test.
				Bit: bit index to wait for clear state, converted to a BYTE mask using (1 << Bit).

			Output:
				None. The current task waits for WaitingForNotBit and PendSV is requested.
	*/
	void WaitForNotBit(volatile BYTE *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotBit;
		SCHEDULE;
	}


	/*					WaitForNotBitTO

			Purpose:
				Suspends the current task until one bit in a BYTE becomes clear or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the BYTE object to test.
				Bit: bit index to wait for clear state, converted to a BYTE mask using (1 << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit became clear before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForNotBitTO(volatile BYTE *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(1<<Bit);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForNotBit

			Purpose:
				Suspends the current task until one bit in a BYTE becomes clear, only if the bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already clear, it returns immediately without requesting scheduling.

			Input:
				Add: pointer to the BYTE object to test.
				Bit: bit index to wait for clear state, converted to a BYTE mask using (1 << Bit).

			Output:
				None. If the bit is set, the current task waits for WaitingForNotBit and PendSV is requested.
	*/
	void CheckAndWaitForNotBit(volatile BYTE *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(!(*Add&(1<<Bit))) return;
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForNotBitTO

			Purpose:
				Suspends the current task until one bit in a BYTE becomes clear or the task timeout expires, only if the bit
				is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already clear, it returns true without requesting scheduling. If Time is not zero, a
				new task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the BYTE object to test.
				Bit: bit index to wait for clear state, converted to a BYTE mask using (1 << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit is already clear or became clear before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForNotBitTO(volatile BYTE *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if(!(*Add&(1<<Bit))) return true;
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_WORD_BIT

	/*					WaitForWordBit

			Purpose:
				Suspends the current task until one bit in a WORD becomes set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the WORD object to test.
				Bit: bit index to wait for, converted to a WORD mask using (1 << Bit).

			Output:
				None. The current task waits for WaitingForWordBit and PendSV is requested.
	*/
	void WaitForWordBit(volatile WORD *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordBit;
		SCHEDULE;
	}

	/*					WaitForWordBitTO

			Purpose:
				Suspends the current task until one bit in a WORD becomes set or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the WORD object to test.
				Bit: bit index to wait for, converted to a WORD mask using (1 << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForWordBitTO(volatile WORD *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
				CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(1<<Bit);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForWordBit

			Purpose:
				Suspends the current task until one bit in a WORD becomes set, only if the bit is currently clear.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already set, it returns immediately without requesting scheduling.

			Input:
				Add: pointer to the WORD object to test.
				Bit: bit index to wait for, converted to a WORD mask using (1 << Bit).

			Output:
				None. If the bit is clear, the current task waits for WaitingForWordBit and PendSV is requested.
	*/
	void CheckAndWaitForWordBit(volatile WORD *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*Add&(1<<Bit)) return;
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForWordBitTO

			Purpose:
				Suspends the current task until one bit in a WORD becomes set or the task timeout expires, only if the bit is
				currently clear.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already set, it returns true without requesting scheduling. If Time is not zero, a new
				task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the WORD object to test.
				Bit: bit index to wait for, converted to a WORD mask using (1 << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit is already set or became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForWordBitTO(volatile WORD *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if(*Add&(1<<Bit)) return true;
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}
#endif

#if WAIT_FOR_NOT_WORD_BIT

	/*					WaitForWordNotBit

			Purpose:
				Suspends the current task until one bit in a WORD becomes clear.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the WORD object to test.
				Bit: bit index to wait for clear state, converted to a WORD mask using (1 << Bit).

			Output:
				None. The current task waits for WaitingForWordNotBit and PendSV is requested.
	*/
	void WaitForWordNotBit(volatile WORD *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordNotBit;
		SCHEDULE;
	}

	/*					WaitForWordNotBitTO

			Purpose:
				Suspends the current task until one bit in a WORD becomes clear or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the WORD object to test.
				Bit: bit index to wait for clear state, converted to a WORD mask using (1 << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit became clear before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForWordNotBitTO(volatile WORD *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(1<<Bit);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForWordNotBit

			Purpose:
				Suspends the current task until one bit in a WORD becomes clear, only if the bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already clear, it returns immediately without requesting scheduling.

			Input:
				Add: pointer to the WORD object to test.
				Bit: bit index to wait for clear state, converted to a WORD mask using (1 << Bit).

			Output:
				None. If the bit is set, the current task waits for WaitingForWordNotBit and PendSV is requested.
	*/
	void CheckAndWaitForWordNotBit(volatile WORD *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(!(*Add&(1<<Bit))) return;
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordNotBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForWordNotBitTO

			Purpose:
				Suspends the current task until one bit in a WORD becomes clear or the task timeout expires, only if the bit
				is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already clear, it returns true without requesting scheduling. If Time is not zero, a
				new task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the WORD object to test.
				Bit: bit index to wait for clear state, converted to a WORD mask using (1 << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit is already clear or became clear before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForWordNotBitTO(volatile WORD *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if(!(*Add&(1<<Bit))) return true;
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(1<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_DWORD_BIT

	/*					WaitForDWordBit

			Purpose:
				Suspends the current task until one bit in a DWORD becomes set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the DWORD object to test.
				Bit: bit index to wait for, converted to a DWORD mask using (1UL << Bit).

			Output:
				None. The current task waits for WaitingForDWordBit and PendSV is requested.
	*/
	void WaitForDWordBit(volatile DWORD *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(1UL<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordBit;
		SCHEDULE;
	}

	/*					WaitForDWordBitTO

			Purpose:
				Suspends the current task until one bit in a DWORD becomes set or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the DWORD object to test.
				Bit: bit index to wait for, converted to a DWORD mask using (1UL << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForDWordBitTO(volatile DWORD *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(1UL<<Bit);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForDWordBit

			Purpose:
				Suspends the current task until one bit in a DWORD becomes set, only if the bit is currently clear.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already set, it returns immediately without requesting scheduling.

			Input:
				Add: pointer to the DWORD object to test.
				Bit: bit index to wait for, converted to a DWORD mask using (1UL << Bit).

			Output:
				None. If the bit is clear, the current task waits for WaitingForDWordBit and PendSV is requested.
	*/
	void CheckAndWaitForDWordBit(volatile DWORD *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*Add&(1<<Bit)) return;
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(1UL<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForDWordBitTO

			Purpose:
				Suspends the current task until one bit in a DWORD becomes set or the task timeout expires, only if the bit is
				currently clear.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already set, it returns true without requesting scheduling. If Time is not zero, a new
				task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the DWORD object to test.
				Bit: bit index to wait for, converted to a DWORD mask using (1UL << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit is already set or became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForDWordBitTO(volatile DWORD *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if(*Add&(1UL<<Bit)) return true;
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(1UL<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_NOT_DWORD_BIT

	/*					WaitForDWordNotBit

			Purpose:
				Suspends the current task until one bit in a DWORD becomes clear.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the DWORD object to test.
				Bit: bit index to wait for clear state, converted to a DWORD mask using (1UL << Bit).

			Output:
				None. The current task waits for WaitingForDWordNotBit and PendSV is requested.
	*/
	void WaitForDWordNotBit(volatile DWORD *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(1UL<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordNotBit;
		SCHEDULE;
	}

	/*					WaitForDWordNotBitTO

			Purpose:
				Suspends the current task until one bit in a DWORD becomes clear or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the DWORD object to test.
				Bit: bit index to wait for clear state, converted to a DWORD mask using (1UL << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit became clear before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForDWordNotBitTO(volatile DWORD *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(1UL<<Bit);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForDWordNotBit

			Purpose:
				Suspends the current task until one bit in a DWORD becomes clear, only if the bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already clear, it returns immediately without requesting scheduling.

			Input:
				Add: pointer to the DWORD object to test.
				Bit: bit index to wait for clear state, converted to a DWORD mask using (1UL << Bit).

			Output:
				None. If the bit is set, the current task waits for WaitingForDWordNotBit and PendSV is requested.
	*/
	void CheckAndWaitForDWordNotBit(volatile DWORD *Add, WORD Bit){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(!(*Add&(1<<Bit))) return;
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(1UL<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordNotBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForDWordNotBitTO

			Purpose:
				Suspends the current task until one bit in a DWORD becomes clear or the task timeout expires, only if the bit
				is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the bit before suspending
				the task; if the bit is already clear, it returns true without requesting scheduling. If Time is not zero, a
				new task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the DWORD object to test.
				Bit: bit index to wait for clear state, converted to a DWORD mask using (1UL << Bit).
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the bit is already clear or became clear before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForDWordNotBitTO(volatile DWORD *Add, WORD Bit, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		if(!(*Add&(1UL<<Bit))) return true;
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(1UL<<Bit);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_SEM

	/*					WaitForSem

			Purpose:
				Suspends the current task until a semaphore can be acquired.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It does not test the semaphore
				before suspending the task; the scheduler evaluates the wait condition with TestAndSet().

			Input:
				S: pointer to the semaphore to wait on.

			Output:
				None. The current task waits for WaitingForSemaphore and PendSV is requested.
	*/
	void WaitForSem(Semaphore *S){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.S=S;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForSemaphore;
		SCHEDULE;
	}

	/*					WaitForSemTO

			Purpose:
				Suspends the current task until a semaphore can be acquired or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It does not test the semaphore
				before suspending the task. If Time is not zero, a new task timer is started; if Time is zero, the previously
				configured task timer is kept.

			Input:
				S: pointer to the semaphore to wait on.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the semaphore was acquired before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForSemTO(Semaphore *S, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.S=S;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForSemaphoreTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForSem

			Purpose:
				Suspends the current task until a semaphore can be acquired, only if it cannot be acquired immediately.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the semaphore with
				TestAndSet() before suspending the task; if the semaphore is acquired immediately, it returns without
				requesting scheduling.

			Input:
				S: pointer to the semaphore to wait on.

			Output:
				None. If the semaphore cannot be acquired immediately, the current task waits for WaitingForSemaphore and
				PendSV is requested.
	*/
	void CheckAndWaitForSem(Semaphore *S){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(TestAndSet(S)!=false) return;
		CurrentTaskPtr->ObjectToWait.S=S;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForSemaphore;
		SCHEDULE;
	}

	/*					CheckAndWaitForSemTO

			Purpose:
				Suspends the current task until a semaphore can be acquired or the task timeout expires, only if the
				semaphore cannot be acquired immediately.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the semaphore with
				TestAndSet() before suspending the task; if the semaphore is acquired immediately, it returns true without
				requesting scheduling. If Time is not zero, a new task timer is started; if Time is zero, the previously
				configured task timer is kept.

			Input:
				S: pointer to the semaphore to wait on.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the semaphore was acquired immediately or before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForSemTO(Semaphore *S, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(TestAndSet(S)!=false) return true;
		CurrentTaskPtr->ObjectToWait.S=S;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForSemaphoreTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_COUNTING_SEM

	/*					WaitForCountingSem

			Purpose:
				Suspends the current task until a counting semaphore can be acquired.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It does not test the counting
				semaphore before suspending the task; the scheduler evaluates the wait condition with GetCountingSem().

			Input:
				S: pointer to the counting semaphore to wait on.

			Output:
				None. The current task waits for WaitingForCountingSem and PendSV is requested.
	*/
	void WaitForCountingSem(T_CountingSem *S){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.CS=S;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForCountingSem;
		SCHEDULE;
	}

	/*					WaitForCountingSemTO

			Purpose:
				Suspends the current task until a counting semaphore can be acquired or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It does not test the counting
				semaphore before suspending the task. If Time is not zero, a new task timer is started; if Time is zero, the
				previously configured task timer is kept.

			Input:
				S: pointer to the counting semaphore to wait on.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the counting semaphore was acquired before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForCountingSemTO(T_CountingSem *S, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.CS=S;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForCountingSemTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForCountingSem

			Purpose:
				Suspends the current task until a counting semaphore can be acquired, only if it cannot be acquired immediately.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the counting semaphore
				with GetCountingSem() before suspending the task; if the semaphore is acquired immediately, it returns
				without requesting scheduling.

			Input:
				S: pointer to the counting semaphore to wait on.

			Output:
				None. If the counting semaphore cannot be acquired immediately, the current task waits for
				WaitingForCountingSem and PendSV is requested.
	*/
	void CheckAndWaitForCountingSem(T_CountingSem *S){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(GetCountingSem(S)) return;
		CurrentTaskPtr->ObjectToWait.CS=S;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForCountingSem;
		SCHEDULE;
	}

	/*					CheckAndWaitForCountingSemTO

			Purpose:
				Suspends the current task until a counting semaphore can be acquired or the task timeout expires, only if the
				semaphore cannot be acquired immediately.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the counting semaphore
				with GetCountingSem() before suspending the task; if the semaphore is acquired immediately, it returns true
				without requesting scheduling. If Time is not zero, a new task timer is started; if Time is zero, the
				previously configured task timer is kept.

			Input:
				S: pointer to the counting semaphore to wait on.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if the counting semaphore was acquired immediately or before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForCountingSemTO(T_CountingSem *S, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(GetCountingSem(S)) return true;
		CurrentTaskPtr->ObjectToWait.CS=S;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForCountingSemTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_ALMENO_UN_BIT

	/*					WaitForAlmenoUnBit

			Purpose:
				Suspends the current task until at least one bit selected by a BYTE mask becomes set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the BYTE object to test.
				Mask: BYTE mask selecting the bits to wait for.

			Output:
				None. The current task waits for WaitingForBit and PendSV is requested.
	*/
	void WaitForAlmenoUnBit(volatile BYTE *Add, BYTE Mask){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(Mask);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBit;
		SCHEDULE;
	}

	/*					WaitForAlmenoUnBitTO

			Purpose:
				Suspends the current task until at least one bit selected by a BYTE mask becomes set or the task timeout
				expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the BYTE object to test.
				Mask: BYTE mask selecting the bits to wait for.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if at least one selected bit became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForAlmenoUnBitTO(volatile BYTE *Add, BYTE Mask, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(Mask);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForAlmenoUnBit

			Purpose:
				Suspends the current task until at least one bit selected by a BYTE mask becomes set, only if no selected bit
				is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if at least one selected bit is already set, it returns immediately without requesting
				scheduling.

			Input:
				Add: pointer to the BYTE object to test.
				Mask: BYTE mask selecting the bits to wait for.

			Output:
				None. If no selected bit is set, the current task waits for WaitingForBit and PendSV is requested.
	*/
	void CheckAndWaitForAlmenoUnBit(volatile BYTE *Add, BYTE Mask){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*Add&Mask) return;
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(Mask);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForAlmenoUnBitTO

			Purpose:
				Suspends the current task until at least one bit selected by a BYTE mask becomes set or the task timeout
				expires, only if no selected bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if at least one selected bit is already set, it returns true without requesting
				scheduling. If Time is not zero, a new task timer is started; if Time is zero, the previously configured task
				timer is kept.

			Input:
				Add: pointer to the BYTE object to test.
				Mask: BYTE mask selecting the bits to wait for.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if at least one selected bit is already set or became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForAlmenoUnBitTO(volatile BYTE *Add, BYTE Mask, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*Add&Mask) return true;
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=(Mask);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}	
	}

#endif

#if WAIT_FOR_NESSUN_BIT

	/*					WaitForNessunBit

			Purpose:
				Suspends the current task until no bit selected by a BYTE mask is set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the BYTE object to test.
				Mask: BYTE mask selecting the bits to wait for clear state.

			Output:
				None. The current task waits for WaitingForNotBit and PendSV is requested.
	*/
	void WaitForNessunBit(volatile BYTE *Add,  BYTE Mask){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=Mask;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotBit;
		SCHEDULE;
	}

	/*					WaitForNessunBitTO

			Purpose:
				Suspends the current task until no bit selected by a BYTE mask is set or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the BYTE object to test.
				Mask: BYTE mask selecting the bits to wait for clear state.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if no selected bit was set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForNessunBitTO(volatile BYTE *Add,  BYTE Mask, DWORD Time){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=Mask;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForNessunBit

			Purpose:
				Suspends the current task until no bit selected by a BYTE mask is set, only if at least one selected bit is
				currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if no selected bit is already set, it returns immediately without requesting scheduling.

			Input:
				Add: pointer to the BYTE object to test.
				Mask: BYTE mask selecting the bits to wait for clear state.

			Output:
				None. If at least one selected bit is set, the current task waits for WaitingForNotBit and PendSV is requested.
	*/
	void CheckAndWaitForNessunBit(volatile BYTE *Add,  BYTE Mask){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(((*Add)&Mask)==0) return;
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=Mask;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForNessunBitTO

			Purpose:
				Suspends the current task until no bit selected by a BYTE mask is set or the task timeout expires, only if at
				least one selected bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if no selected bit is already set, it returns true without requesting scheduling. If Time
				is not zero, a new task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the BYTE object to test.
				Mask: BYTE mask selecting the bits to wait for clear state.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if no selected bit is already set or all selected bits became clear before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForNessunBitTO(volatile BYTE *Add,  BYTE Mask, DWORD Time){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(((*Add)&Mask)==0) return true;
		CurrentTaskPtr->ObjectToWait.C=Add;
		CurrentTaskPtr->Param.B_Param=Mask;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_ALMENO_UN_WORD_BIT

	/*					WaitForAlmenoUnWordBit

			Purpose:
				Suspends the current task until at least one bit selected by a WORD mask becomes set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the WORD object to test.
				Mask: WORD mask selecting the bits to wait for.

			Output:
				None. The current task waits for WaitingForWordBit and PendSV is requested.
	*/
	void WaitForAlmenoUnWordBit(volatile WORD *Add, WORD Mask){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(Mask);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordBit;
		SCHEDULE;
	}

	/*					WaitForAlmenoUnWordBitTO

			Purpose:
				Suspends the current task until at least one bit selected by a WORD mask becomes set or the task timeout
				expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the WORD object to test.
				Mask: WORD mask selecting the bits to wait for.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if at least one selected bit became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForAlmenoUnWordBitTO(volatile WORD *Add, WORD Mask, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(Mask);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForAlmenoUnWordBit

			Purpose:
				Suspends the current task until at least one bit selected by a WORD mask becomes set, only if no selected bit
				is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if at least one selected bit is already set, it returns immediately without requesting
				scheduling.

			Input:
				Add: pointer to the WORD object to test.
				Mask: WORD mask selecting the bits to wait for.

			Output:
				None. If no selected bit is set, the current task waits for WaitingForWordBit and PendSV is requested.
	*/
	void CheckAndWaitForAlmenoUnWordBit(volatile WORD *Add, WORD Mask){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*Add&Mask) return;
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=Mask;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForAlmenoUnWordBitTO

			Purpose:
				Suspends the current task until at least one bit selected by a WORD mask becomes set or the task timeout
				expires, only if no selected bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if at least one selected bit is already set, it returns true without requesting
				scheduling. If Time is not zero, a new task timer is started; if Time is zero, the previously configured task
				timer is kept.

			Input:
				Add: pointer to the WORD object to test.
				Mask: WORD mask selecting the bits to wait for.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if at least one selected bit is already set or became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForAlmenoUnWordBitTO(volatile WORD *Add, WORD Mask, DWORD Time){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*Add&Mask) return true;
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=Mask;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_NESSUN_WORD_BIT

	/*					WaitForNessunWordBit

			Purpose:
				Suspends the current task until no bit selected by a WORD mask is set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the WORD object to test.
				Mask: WORD mask selecting the bits to wait for clear state.

			Output:
				None. The current task waits for WaitingForWordNotBit and PendSV is requested.
	*/
	void WaitForNessunWordBit(volatile WORD *Add,  WORD Mask){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(Mask);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordNotBit;
		SCHEDULE;
	}

	/*					WaitForNessunWordBitTO

			Purpose:
				Suspends the current task until no bit selected by a WORD mask is set or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the WORD object to test.
				Mask: WORD mask selecting the bits to wait for clear state.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if no selected bit was set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForNessunWordBitTO(volatile WORD *Add,  WORD Mask, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=(Mask);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForNessunWordBit

			Purpose:
				Suspends the current task until no bit selected by a WORD mask is set, only if at least one selected bit is
				currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if no selected bit is already set, it returns immediately without requesting scheduling.

			Input:
				Add: pointer to the WORD object to test.
				Mask: WORD mask selecting the bits to wait for clear state.

			Output:
				None. If at least one selected bit is set, the current task waits for WaitingForWordNotBit and PendSV is
				requested.
	*/
	void CheckAndWaitForNessunWordBit(volatile WORD *Add,  WORD Mask){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(((*Add)&Mask)==0) return;
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=Mask;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordNotBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForNessunWordBitTO

			Purpose:
				Suspends the current task until no bit selected by a WORD mask is set or the task timeout expires, only if at
				least one selected bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if no selected bit is already set, it returns true without requesting scheduling. If Time
				is not zero, a new task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the WORD object to test.
				Mask: WORD mask selecting the bits to wait for clear state.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if no selected bit is already set or all selected bits became clear before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForNessunWordBitTO(volatile WORD *Add,  WORD Mask, DWORD Time){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(((*Add)&Mask)==0) return true;
		CurrentTaskPtr->ObjectToWait.W=Add;
		CurrentTaskPtr->Param.W_Param=Mask;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForWordNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_ALMENO_UN_DWORD_BIT

	/*					WaitForAlmenoUnDWordBit

			Purpose:
				Suspends the current task until at least one bit selected by a DWORD mask becomes set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the DWORD object to test.
				Mask: DWORD mask selecting the bits to wait for.

			Output:
				None. The current task waits for WaitingForDWordBit and PendSV is requested.
	*/
	void WaitForAlmenoUnDWordBit(volatile DWORD *Add, DWORD Mask){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(Mask);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordBit;
		SCHEDULE;
	}

	/*					WaitForAlmenoUnDWordBitTO

			Purpose:
				Suspends the current task until at least one bit selected by a DWORD mask becomes set or the task timeout
				expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the DWORD object to test.
				Mask: DWORD mask selecting the bits to wait for.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if at least one selected bit became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForAlmenoUnDWordBitTO(volatile DWORD *Add, DWORD Mask, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(Mask);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForAlmenoUnDWordBit

			Purpose:
				Suspends the current task until at least one bit selected by a DWORD mask becomes set, only if no selected
				bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if at least one selected bit is already set, it returns immediately without requesting
				scheduling.

			Input:
				Add: pointer to the DWORD object to test.
				Mask: DWORD mask selecting the bits to wait for.

			Output:
				None. If no selected bit is set, the current task waits for WaitingForDWordBit and PendSV is requested.
	*/
	void CheckAndWaitForAlmenoUnDWordBit(volatile DWORD *Add, DWORD Mask){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*Add&Mask) return;
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(Mask);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForAlmenoUnDWordBitTO

			Purpose:
				Suspends the current task until at least one bit selected by a DWORD mask becomes set or the task timeout
				expires, only if no selected bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if at least one selected bit is already set, it returns true without requesting
				scheduling. If Time is not zero, a new task timer is started; if Time is zero, the previously configured task
				timer is kept.

			Input:
				Add: pointer to the DWORD object to test.
				Mask: DWORD mask selecting the bits to wait for.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if at least one selected bit is already set or became set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForAlmenoUnDWordBitTO(volatile DWORD *Add, DWORD Mask, DWORD Time){
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(*Add&Mask) return true;
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=(Mask);
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif

#if WAIT_FOR_NESSUN_DWORD_BIT

	/*					WaitForNessunDWordBit

			Purpose:
				Suspends the current task until no bit selected by a DWORD mask is set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. The wait condition is evaluated by
				the scheduler.

			Input:
				Add: pointer to the DWORD object to test.
				Mask: DWORD mask selecting the bits to wait for clear state.

			Output:
				None. The current task waits for WaitingForDWordNotBit and PendSV is requested.
	*/
	void WaitForNessunDWordBit(volatile DWORD *Add,  DWORD Mask){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordNotBit;
		CurrentTaskPtr->Param.DW_Param=Mask;
		SCHEDULE;
	}

	/*					WaitForNessunDWordBitTO

			Purpose:
				Suspends the current task until no bit selected by a DWORD mask is set or the task timeout expires.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. If Time is not zero, a new task
				timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the DWORD object to test.
				Mask: DWORD mask selecting the bits to wait for clear state.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if no selected bit was set before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool WaitForNessunDWordBitTO(volatile DWORD *Add,  DWORD Mask, DWORD Time){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=Mask;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

	/*					CheckAndWaitForNessunDWordBit

			Purpose:
				Suspends the current task until no bit selected by a DWORD mask is set, only if at least one selected bit is
				currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if no selected bit is already set, it returns immediately without requesting scheduling.

			Input:
				Add: pointer to the DWORD object to test.
				Mask: DWORD mask selecting the bits to wait for clear state.

			Output:
				None. If at least one selected bit is set, the current task waits for WaitingForDWordNotBit and PendSV is
				requested.
	*/
	void CheckAndWaitForNessunDWordBit(volatile DWORD *Add,  DWORD Mask){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(((*Add)&Mask)==0) return;
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=Mask;
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordNotBit;
		SCHEDULE;
	}

	/*					CheckAndWaitForNessunDWordBitTO

			Purpose:
				Suspends the current task until no bit selected by a DWORD mask is set or the task timeout expires, only if
				at least one selected bit is currently set.

			Author:
				Paolo Rozzi

			Reviewer:
				---

			Context:
				This is a public RTK wait function and must be called from task context. It checks the masked bits before
				suspending the task; if no selected bit is already set, it returns true without requesting scheduling. If Time
				is not zero, a new task timer is started; if Time is zero, the previously configured task timer is kept.

			Input:
				Add: pointer to the DWORD object to test.
				Mask: DWORD mask selecting the bits to wait for clear state.
				Time: timeout in system ticks, or zero to keep the currently configured task timer.

			Output:
				true if no selected bit is already set or all selected bits became clear before the timeout expired.
				false if the wait ended because the timeout expired.
	*/
	bool CheckAndWaitForNessunDWordBitTO(volatile DWORD *Add,  DWORD Mask, DWORD Time){
		// PIPPO PIPPO PIPPO Da testare
		#if CALLER_ADDRESS
			CurrentTaskPtr->WaitCallerAddress=(void *)__get_LR();
		#endif
		if(((*Add)&Mask)==0) return true;
		CurrentTaskPtr->ObjectToWait.DW=Add;
		CurrentTaskPtr->Param.DW_Param=Mask;
		if(Time) SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
		CurrentTaskPtr->TaskStatus.AsByte=WaitingForDWordNotBitTO;
		SCHEDULE;
		__disable_irq();
		if(CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone){
			CurrentTaskPtr->TaskStatus.AsByte=WaitingForNone;
			__enable_irq();
			return false;
		}
		else{
			__enable_irq();
			return true;
		}
	}

#endif
