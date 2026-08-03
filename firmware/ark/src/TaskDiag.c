/*						ARK Project - Adaptive Runtime Kernel

	Module:
		TaskDiag.c

	Purpose:
		RTK task diagnostic snapshot implementation.

	Description:
		This module collects diagnostic information from RTK task descriptors, including priority,
		wait state, wait object, timeout, execution counter, idle time, label and estimated unused
		stack space when the corresponding diagnostic options are enabled.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK task diagnostic implementations.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include "TaskDiag.h"
#include "TimerTic.h" 
#include "MM.h"
 
WORD GetDescriptorsPointers(T_TaskDescriptor *Ptrs[], WORD MaxTasks, T_TaskDescriptor *Lista){
	WORD i=0;
	uint32_t SchedulerLock=RTK_SchedulerLock();
	if((Ptrs[0]=Lista)!=0){
		i=1;
		while(Ptrs[i-1]->Next!=Ptrs[0]){
			Ptrs[i]=Ptrs[i-1]->Next;
			i++;
			if(i>=MaxTasks)
				break;
		}
	}  
	RTK_Unlock(SchedulerLock);
	if(i>1)
		for(unsigned j=0; j<i-1; j++)
			for(unsigned k=j+1; k<i; k++)
				if(Ptrs[j]->Label<Ptrs[k]->Label){
					T_TaskDescriptor *P=Ptrs[j];
					Ptrs[j]=Ptrs[k];
					Ptrs[k]=P;
				}
	return i; 
}
  
/*        GetTaskDiagStatus
   Riempie la struttura con lo stato della task di handler passato. Se la task
non esiste torna false.
*/
void GetTaskDiagStatus(T_TaskDescriptor *P, T_TaskDiagStatus *TaskDiagStatus){
	uint32_t SchedulerLock=RTK_SchedulerLock();
	TaskDiagStatus->TaskPriority=P->TaskPriority;
	TaskDiagStatus->WaitingType=P->TaskStatus.AsByte;
	TaskDiagStatus->StopAddress=P->PSP->PC;
	#if CALLER_ADDRESS
		TaskDiagStatus->StopAddress=(int)P->PSP->PC;
		if(TaskDiagStatus->WaitingType!=WaitingForNone){
			TaskDiagStatus->StopAddress=(int)P->WaitCallerAddress;
		}
	#endif
	if((TaskDiagStatus->WaitingType&0x80)||(TaskDiagStatus->WaitingType==WaitingForTime))
		TaskDiagStatus->TimeToWait=TimerTicQuantoManca(&P->Time);
	else
		TaskDiagStatus->TimeToWait=0;
	TaskDiagStatus->AddressOfWaitingObject=(void *)P->ObjectToWait.DW;
	TaskDiagStatus->WaitingParam=P->Param.DW_Param;
	#if EXECUTION_CTR
		TaskDiagStatus->RunCtr=P->TaskCtr;
	#else
		TaskDiagStatus->RunCtr=0xAA55;
	#endif
	#if IDLE_TIME
		TaskDiagStatus->IdleTime=TimerCtr-P->TimerCtrAtLastSched;
	#else
		TaskDiagStatus->IdleTime=0xAA55AA55;
	#endif
	#if TASK_LABEL != 0
		TaskDiagStatus->Label=P->Label;
	#endif
	#if EVALUATE_FREE_STACK
		TaskDiagStatus->MinUnusedStackDWords=0;
		while(((DWORD *)(&P[1]))[TaskDiagStatus->MinUnusedStackDWords]==STACK_FILL_PATTERN)
			TaskDiagStatus->MinUnusedStackDWords++;
	#else
		TaskDiagStatus->MinUnusedStackDWords=0xFFFF;
	#endif
	RTK_Unlock(SchedulerLock);
}

