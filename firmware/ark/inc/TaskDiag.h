/*						ARK Project - Adaptive Runtime Kernel

	Module:
		TaskDiag.h

	Purpose:
		Public interface and data structures for RTK task diagnostics.

	Description:
		This header defines the task diagnostic snapshot structure and declares services used to
		enumerate task descriptors and collect diagnostic state from a selected task.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK task diagnostic headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __TaskDiag_h
	#define __TaskDiag_h

	#include "Sched.h"
	#include "Type.h"
 
	/*				T_TaskDiagStatus
			StopAddress contains the user-code return address: PC when the task is not waiting, and LR
		(the caller address) in the other cases.
			WaitingParam contains the parameter associated with the wait operation, for example the mask
		used by a wait-for-bit operation.
	*/
	typedef struct{
		DWORD StopAddress;	// user-code return address: LR or PC, depending if the task is in wait mode or not.
		DWORD TimeToWait;	// remaining wait if sospension time out is enabled
		DWORD IdleTime;		//
		void *AddressOfWaitingObject;
		DWORD WaitingParam;
		WORD RunCtr;		//
		WORD MinUnusedStackDWords;
		T_Text Label;
		BYTE TaskPriority;
		BYTE WaitingType;
	}T_TaskDiagStatus;
 
	#ifdef __cplusplus
		extern "C" {
	#endif

	void DiagTask(void);
	WORD GetDescriptorsPointers(T_TaskDescriptor *Ptrs[], WORD MaxTasks, T_TaskDescriptor *Lista);
	void GetTaskDiagStatus(T_TaskDescriptor *P, T_TaskDiagStatus *TaskDiagStatus);

	#ifdef __cplusplus
		}
	#endif

#endif
