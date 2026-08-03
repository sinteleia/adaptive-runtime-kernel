/*						ARK Project - Adaptive Runtime Kernel
	Module:
		Sched.c
	Purpose:
		RTK scheduler implementation.
	Description:
		This module implements scheduler initialization, task creation, task selection, priority-list
		management, SysTick handling and scheduler start/stop paths. It coordinates RTK task descriptors,
		tic objects, timer servicing and the PendSV context-switch path implemented in assembly.
	ARK version:
		1.0
	File revision:
		1.0
	Origin:
		Derived from older RTK scheduler versions.
	Author:
		Paolo Rozzi
	Reviewer:
		---
*/
#define RTK_CONFIG_REPORT_MISSING_USR_OPT
#include "Sched.h"
#include "General.h"
#include "TimerTic.h"
#include "Pack16.h"
#include "WFill.h"
#include "Tic.h"
#include "MyIntrinsics.h"
#include "mm.h"
#include "RTK_Interface.h"

#define IDLE_TASK_STACK_SIZE 500

#if defined(__VFP_FP__) && !defined(__SOFTFP__) && !defined(CONTROL_FPCA_Msk)
	#define CONTROL_FPCA_Msk (1UL<<2U)
#endif

#define ACTIVE_SCHED_CTR

#ifdef ACTIVE_SCHED_CTR
	DWORD SchedulazioniAttive;
#endif

BYTE TicPerTau;
BYTE MediumForLow;
BYTE PriorityInversionCtr;
BYTE TauCtr;
volatile bool KernelRunning;	// Indicates whether the scheduler has already been started
Func IdleFunctionPtr;

#if TIC_OBJs
static WORD SchedulerMaxSchedRoutines;
static WORD SchedulerMaxISR_Routines;
#endif

bool SchedulazioneCompleta;
bool DeleteCurrentTask;
uint32_t RTK_SchedulerBasepri;
uint32_t RTK_SysTicBasepri;

volatile T_TaskDescriptor *IdleTaskDescriptor;
volatile T_TaskDescriptor *CriticalProcList;
volatile T_TaskDescriptor *HiPriProcList;
volatile T_TaskDescriptor *MediumPriProcList;
volatile T_TaskDescriptor *LowPriProcList;
volatile T_TaskDescriptor *BkGroundProcList;
volatile T_TaskDescriptor *CurrentTaskPtr;
T_TaskDescriptor ExitTask;	// Task descriptor where SchedulerStart() saves the context of the first SCHEDULE invocation,
							// so execution resumes from that point after all tasks have terminated.

#ifdef CONTROL_FPCA_Msk
typedef struct {
	float S[32];
	DWORD FPSCR;
} T_SchedulerFpContext;
#endif

//	Local prototypes
bool ScanProcList(T_TaskDescriptor *ProcList);
volatile T_TaskDescriptor *FirstToRun(void);
void SysTick_Handler(void);

/*						IdleTask
	Purpose:
		Default weak idle task.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Used when the application passes IdleTask to SchedulerInit() and does not provide a stronger implementation.
	Input:
		None.
	Output:
		Does not return.
	Notes:
		The task waits for an interrupt, then explicitly requests scheduling before going back to sleep if no task is ready.
*/
__attribute__((weak)) void IdleTask(void){
	while(1){
		__asm volatile("wfi");
		SCHEDULE;
	}
}

/*					SchedulerInit
	Purpose:
		Initializes the scheduler configuration.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This function must be called while RTK is stopped and at least once before SchedulerStart() can be used.
		The stored configuration can be reused by one or more SchedulerStart() cycles.
	Input:
		IdleTask: idle task entry point.
		ParTicPerTau: number of system ticks used to generate one tau.
		ParMediumForLow: medium/low scheduling ratio.
		MaxSchedRoutines: maximum number of scheduler tic objects, when TIC_OBJs is enabled.
		MaxISR_Routines: maximum number of ISR tic objects, when TIC_OBJs is enabled.
	Output:
		true if the scheduler configuration has been stored.
*/
bool SchedulerInit(Func IdleTask, BYTE ParTicPerTau, BYTE ParMediumForLow
#if TIC_OBJs
                   ,WORD MaxSchedRoutines, WORD MaxISR_Routines
#endif
                  ){
	KernelRunning=false;
	RTK_SchedulerBasepri=RTK_GetSchedulerBasepri();
	RTK_SysTicBasepri=RTK_GetSysTicBasepri();
	IdleFunctionPtr=IdleTask;
	IdleTaskDescriptor=NULL;
	TicPerTau=TauCtr=ParTicPerTau;
	MediumForLow=PriorityInversionCtr=ParMediumForLow;
	InitTimerTic();
	CriticalProcList=HiPriProcList=MediumPriProcList=
	LowPriProcList=BkGroundProcList=NULL;
	#if TIC_OBJs
		SchedulerMaxSchedRoutines=MaxSchedRoutines;
		SchedulerMaxISR_Routines=MaxISR_Routines;
	#endif
	#ifdef ACTIVE_SCHED_CTR
		SchedulazioniAttive=0;
	#endif
	return true;
}

/*					SchedulerStart
	Purpose:
		Starts one scheduler execution cycle.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This function must be called while RTK is stopped. SchedulerInit() must have stored a valid idle task entry point
		before this function is called. At least one user task should normally exist before the start; if no user task
		can run, the scheduler returns to the caller.
		The function can be called again after a regular return without repeating SchedulerInit().
	Input:
		None.
	Output:
		false if the scheduler cannot be started because the idle function is missing, the idle task already exists, the
		idle task allocation fails, or tic object initialization fails.
		true after a regular scheduler cycle, when all user tasks have terminated and RTK has restored its stopped state.
*/
bool SchedulerStart(){
	// Scheduler can not create the Idle task without an Idle function;
	// Idle task can not be present when the scheduler is not started.
	if(IdleFunctionPtr==NULL || IdleTaskDescriptor!=NULL) return false;
	size_t l=((IDLE_TASK_STACK_SIZE<<1)+sizeof(T_RegisterFile)+sizeof(T_TaskDescriptor)+7)&0xFFFFFFF8;
	IdleTaskDescriptor=(T_TaskDescriptor *)malloc(l);
	if(IdleTaskDescriptor==NULL) return false;
	#if EVALUATE_FREE_STACK
		DWordFill(STACK_FILL_PATTERN, (DWORD *)IdleTaskDescriptor, l>>2);
	#endif
	#if STACK_GUARD
		IdleTaskDescriptor->StackGuard=STACK_GUARD_PATTERN;
	#endif
	IdleTaskDescriptor->PSP=(T_RegisterFile*)(((DWORD)IdleTaskDescriptor+l-sizeof(T_RegisterFile))&0xFFFFFFF8);
	IdleTaskDescriptor->PSP->xPSR=0;
	IdleTaskDescriptor->PSP->ExceptionNumber=0;
	IdleTaskDescriptor->PSP->Tumb=1;
	IdleTaskDescriptor->PSP->PC=(DWORD)IdleFunctionPtr;
	IdleTaskDescriptor->R14=0xFFFFFFFD;    // Return from interrupt with PSP
	IdleTaskDescriptor->TaskStatus.AsByte=0;
	IdleTaskDescriptor->TaskPriority=TaskPriorityIdle;
	#if TASK_LABEL != 0
		IdleTaskDescriptor->Label=RTK_Pack("IDLE TASK   ");
	#endif
	#if EXECUTION_CTR
		IdleTaskDescriptor->TaskCtr=0;
	#endif
	#if IDLE_TIME
		IdleTaskDescriptor->TimerCtrAtLastSched=TimerCtr;
	#endif
	IdleTaskDescriptor->Next=NULL;
	#if TIC_OBJs
		if(!InitTicObjects(SchedulerMaxSchedRoutines, SchedulerMaxISR_Routines)){
			free((void *)IdleTaskDescriptor);
			IdleTaskDescriptor=NULL;
			return false;
		}
	#endif
	InitTimerTic();
	#ifdef CONTROL_FPCA_Msk
		T_SchedulerFpContext SchedulerFpContext;
		uint32_t SchedulerControl;
		bool SchedulerHadFpContext;
	#endif
	// Schedule out the current task (ExitTask)
	START_PROTECTION;
	#if STACK_GUARD
		ExitTask.StackGuard=STACK_GUARD_PATTERN;
	#endif
	CurrentTaskPtr=&ExitTask;
	#ifdef CONTROL_FPCA_Msk
		SchedulerControl=__get_CONTROL();
		SchedulerHadFpContext=(SchedulerControl&CONTROL_FPCA_Msk)!=0U;
		if(SchedulerHadFpContext){
			__asm volatile("vstmia %0, {s0-s31}" : : "r"(SchedulerFpContext.S) : "memory");
			__asm volatile("vmrs %0, fpscr" : "=r"(SchedulerFpContext.FPSCR));
			__set_CONTROL(SchedulerControl&~CONTROL_FPCA_Msk);
			__DSB();
			__ISB();
		}
	#endif
	SetPriorityPENDVS();		// Initialize the PendSV interrupt
	SetPrioritySysTic();		// The system tick priority is selected by a weak function
	AttivaIlTic();				// Initialize the system tick
	__DSB();
	__ISB();
	TauCtr=TicPerTau;
	PriorityInversionCtr=MediumForLow;
	DeleteCurrentTask=false;
	#ifdef ACTIVE_SCHED_CTR
		SchedulazioniAttive=0;
	#endif
	KernelRunning=true;
	__enable_irq();
	SCHEDULE;					// !!! kernel start now !!!
	// Returning from scheduling means that all tasks have terminated and the scheduler cycle is complete.
	// Restore a state as close as possible to the microprocessor's original state, at least for the resources used by the kernel.
	__disable_irq();
	DisattivaIlTic();            // Disable SysTick IRQ
	ResetPrioritySysTic();       // Restore the default priority values for the
	ResetPriorityPENDVS();       // interrupts used by RTK
	KernelRunning=false;
	#if TIC_OBJs
		DeinitTicObjects();
	#endif
	free((void *)IdleTaskDescriptor);
	IdleTaskDescriptor=NULL;
	__DSB();
	__ISB();
	#ifdef CONTROL_FPCA_Msk
		if(SchedulerHadFpContext){
			__asm volatile("vldmia %0, {s0-s31}" : : "r"(SchedulerFpContext.S) : "memory");
			__asm volatile("vmsr fpscr, %0" : : "r"(SchedulerFpContext.FPSCR));
			__set_CONTROL(SchedulerControl);
			__ISB();
		}
	#endif
	END_PROTECTION;    // Restore the original interrupt-enable state
	return true;
}

#if SCHEDULE_DIAG
	short int LastResumeCause;
	volatile T_TaskDescriptor *LastResumedTask;
#endif

#if SCHEDULE_DIAG
	#define RESUME_CAUSE(X) LastResumeCause=X; LastResumedTask=CurrentTaskPtr;
#else
	#define RESUME_CAUSE(X)
#endif


/*					ScanProcList
	Purpose:
		Scans one task descriptor list to find the next task ready to run.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is an internal scheduler function and is not part of the public RTK API. It is called by FirstToRun() while selecting 
		the next task. The function can be customized to add application-specific or hardware-specific wait conditions.
	Input:
		ProcList: pointer to the circular task descriptor list to scan. It can be NULL.
	Output:
		true if a ready task is found; CurrentTaskPtr is set to that task.
		false if the list is empty or no task in the list is ready.
		If the task is ready because of a timeout, TaskStatus is left unchanged; otherwise TaskStatus is cleared.
*/
bool ScanProcList(T_TaskDescriptor *ProcList){
	if(ProcList){
		CurrentTaskPtr=ProcList;
		do{
			CurrentTaskPtr=CurrentTaskPtr->Next;
			if(CurrentTaskPtr->TaskStatus.AsBit.WaitingWithTimeOut){
				if(IS_TIMER_ELAPSED(CurrentTaskPtr->Time)){
					#if SCHEDULE_DIAG
						LastResumeCause=0x80;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
			}
			switch(CurrentTaskPtr->TaskStatus.AsBit.WaitingFor){
				case WaitingForNone:
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForNone;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				case WaitingForTime:
					if(IS_TIMER_ELAPSED(CurrentTaskPtr->Time)){
						CurrentTaskPtr->TaskStatus.AsByte=0;
						#if SCHEDULE_DIAG
							LastResumeCause=WaitingForTime;
							LastResumedTask=CurrentTaskPtr;
						#endif
						return true;
					}
					break;
				case WaitingForever:
					break;
				case WaitingForSemaphore:
					if(TestAndSet(CurrentTaskPtr->ObjectToWait.S)!=false){
						CurrentTaskPtr->TaskStatus.AsByte=0;
						#if SCHEDULE_DIAG
							LastResumeCause=WaitingForSemaphore;
							LastResumedTask=CurrentTaskPtr;
						#endif
						return true;
					}
				break;
			case WaitingForCountingSem:
				if(GetCountingSem(CurrentTaskPtr->ObjectToWait.CS)){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForCountingSem;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForQueGet:
				if(IS_QUE_PTR_NOT_EMPTY(CurrentTaskPtr->ObjectToWait.Q)){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForQueGet;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForQueEmpty:
				if(IS_QUE_PTR_EMPTY(CurrentTaskPtr->ObjectToWait.Q)){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForQueEmpty;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForBynaryLenQuePut:
				if(IS_BYNARY_LEN_QUE_PTR_NOT_FULL(CurrentTaskPtr->ObjectToWait.BQ)){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForBynaryLenQuePut;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForFreeLenQuePut:
				if(IS_FREE_LEN_QUE_PTR_NOT_FULL(CurrentTaskPtr->ObjectToWait.FQ)){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForFreeLenQuePut;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForFlag:
				if(*CurrentTaskPtr->ObjectToWait.F){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForFlag;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForNotFlag:
				if(!*CurrentTaskPtr->ObjectToWait.F){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForNotFlag;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForBit:
				if(((*CurrentTaskPtr->ObjectToWait.C)&CurrentTaskPtr->Param.B_Param)!=0){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForBit;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForNotBit:
				if(((*CurrentTaskPtr->ObjectToWait.C)&CurrentTaskPtr->Param.B_Param)==0){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForNotBit;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForWordBit:
				if(((*CurrentTaskPtr->ObjectToWait.W)&CurrentTaskPtr->Param.W_Param)!=0){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForWordBit;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForDWordNotBit:
				if(((*CurrentTaskPtr->ObjectToWait.DW)&CurrentTaskPtr->Param.DW_Param)==0){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForDWordNotBit;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;

			case WaitingForDWordBit:
				if(((*CurrentTaskPtr->ObjectToWait.DW)&CurrentTaskPtr->Param.DW_Param)!=0){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForDWordBit;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
			case WaitingForWordNotBit:
				if(((*CurrentTaskPtr->ObjectToWait.W)&CurrentTaskPtr->Param.W_Param)==0){
					CurrentTaskPtr->TaskStatus.AsByte=0;
					#if SCHEDULE_DIAG
						LastResumeCause=WaitingForWordNotBit;
						LastResumedTask=CurrentTaskPtr;
					#endif
					return true;
				}
				break;
		}
			if(CurrentTaskPtr->TaskStatus.AsBit.WaitingWithTimeOut){
				if(IS_TIMER_ELAPSED(CurrentTaskPtr->Time)){
					RESUME_CAUSE(0x80)
					return true;
				}
			}
		}while(CurrentTaskPtr!=ProcList);
	}
	return false;
}

/*					FirstToRun
	Purpose:
		Selects the next task to run.
		If DeleteCurrentTask is set, the current task is destroyed before selecting the next one.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is an internal scheduler function and is not part of the public RTK API. It is called from the context switch
		path after a scheduling request.
	Input:
		None. The function uses the scheduler task lists, CurrentTaskPtr, DeleteCurrentTask, SchedulazioneCompleta and
		PriorityInversionCtr global state.
	Output:
		Pointer to the selected task descriptor. CurrentTaskPtr is updated with the same value.
		Critical and high priority task lists are scanned first. Medium and low priority lists are scanned according to
		the configured inversion ratio; background tasks are scanned last.
		IdleTaskDescriptor is returned when at least one task exists but none is ready to run.
		ExitTask is returned when no user task remains in the scheduler lists.
*/
volatile T_TaskDescriptor *FirstToRun(){
	if(DeleteCurrentTask){
		DeleteCurrentTask=false;
		T_TaskDescriptor *ToDelete=(T_TaskDescriptor *)CurrentTaskPtr;
		CurrentTaskPtr=IdleTaskDescriptor;		
		KillTask(ToDelete);
	}
	if(ScanProcList((T_TaskDescriptor *)CriticalProcList))   // If a critical-priority task is ready,
		CriticalProcList=(T_TaskDescriptor *)CurrentTaskPtr;    // run it.
	else if(ScanProcList((T_TaskDescriptor *)HiPriProcList)) // Otherwise, if a high-priority task is ready,
		HiPriProcList=CurrentTaskPtr;       // run it.
	else if((SchedulazioneCompleta)||(CurrentTaskPtr==IdleTaskDescriptor)||
	        (CurrentTaskPtr->TaskStatus.AsByte!=WaitingForNone)){
		SchedulazioneCompleta=false;
		if(!(PriorityInversionCtr--)){      // Inverted priority order
			PriorityInversionCtr=MediumForLow;
			if(ScanProcList((T_TaskDescriptor *)LowPriProcList))
				LowPriProcList=CurrentTaskPtr;
			else if(ScanProcList((T_TaskDescriptor *)MediumPriProcList))
				MediumPriProcList=CurrentTaskPtr;
			else if(ScanProcList((T_TaskDescriptor *)BkGroundProcList))
				BkGroundProcList=CurrentTaskPtr;
			else if((DWORD)BkGroundProcList|(DWORD)LowPriProcList|(DWORD)MediumPriProcList|
				    (DWORD)HiPriProcList|(DWORD)CriticalProcList)
				CurrentTaskPtr=IdleTaskDescriptor;
			else
				CurrentTaskPtr=&ExitTask;
		}
		else{	// Normal priority order
			if(ScanProcList((T_TaskDescriptor *)MediumPriProcList))
				MediumPriProcList=CurrentTaskPtr;
			else if(ScanProcList((T_TaskDescriptor *)LowPriProcList))
				LowPriProcList=CurrentTaskPtr;
			else if(ScanProcList((T_TaskDescriptor *)BkGroundProcList))
				BkGroundProcList=CurrentTaskPtr;
			else if((DWORD)BkGroundProcList|(DWORD)LowPriProcList|(DWORD)MediumPriProcList|
			        (DWORD)HiPriProcList|(DWORD)CriticalProcList)
				CurrentTaskPtr=IdleTaskDescriptor;
			else
				CurrentTaskPtr=&ExitTask;
		}
	}
	#if IDLE_TIME
		CurrentTaskPtr->TimerCtrAtLastSched=TimerCtr;
	#endif
	#ifdef ACTIVE_SCHED_CTR
		if(CurrentTaskPtr!=IdleTaskDescriptor)
			SchedulazioniAttive++;
	#endif
	#if EXECUTION_CTR
		CurrentTaskPtr->TaskCtr++;
	#endif

	return CurrentTaskPtr;
}

/*					CreateTask
	Purpose:
		Creates a task and inserts it in the scheduler list selected by its priority.
		The new task starts in ready state.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK task creation function. It can be called while RTK is stopped to prepare the next SchedulerStart() 
		cycle, or while RTK is running to create a task dynamically.
	Input:
		Task: task entry point.
		StkSize: task stack size, expressed in WORD units.
		Priority: task scheduling priority.
	Output:
		Pointer to the created task descriptor, or NULL if the descriptor and stack allocation fails.
 */
T_TaskDescriptor *CreateTask(Func Task, WORD StkSize, T_TaskPriority Priority){
	T_TaskDescriptor *NewTask;
	T_TaskDescriptor **Chain;
	size_t l=((StkSize<<1)+sizeof(T_RegisterFile)+sizeof(T_TaskDescriptor)+7)&0xFFFFFFF8;
	if((NewTask=(T_TaskDescriptor *)malloc(l))!=NULL){
		#if EVALUATE_FREE_STACK
			DWordFill(STACK_FILL_PATTERN, (DWORD *)NewTask, l>>2);
		#endif
		#if TASK_LABEL == 16
			NewTask->Label=ALL_INVALID_16;  // ???
		#elif TASK_LABEL == 32
			NewTask->Label=ALL_INVALID_32;
		#elif TASK_LABEL == 64
			NewTask->Label=ALL_INVALID_64;
		#endif
		#if STACK_GUARD
			NewTask->StackGuard=STACK_GUARD_PATTERN;
		#endif
		NewTask->PSP=(T_RegisterFile*)(((DWORD)NewTask+l-sizeof(T_RegisterFile))&0xFFFFFFF8);
		NewTask->PSP->LR=(DWORD)Terminate;
		NewTask->PSP->xPSR=0;
		NewTask->PSP->ExceptionNumber=0;
		NewTask->PSP->Tumb=1;
		NewTask->PSP->PC=(DWORD)Task |1;	// Bit 0 selects the Thumb instruction set
		NewTask->TaskStatus.AsByte=0;
		NewTask->TaskPriority=Priority;
		NewTask->R14=0xFFFFFFFD;			// Return from interrupt with PSP
		InitTimer(&NewTask->Time);
		#if EXECUTION_CTR
			NewTask->TaskCtr=0;
		#endif
		#if IDLE_TIME
			NewTask->TimerCtrAtLastSched=TimerCtr;
		#endif
		switch(Priority){
			case TaskPriorityCritical: Chain=(T_TaskDescriptor **)&CriticalProcList; break;
			case TaskPriorityHi: Chain=(T_TaskDescriptor **)&HiPriProcList; break;
			case TaskPriorityMedium: Chain=(T_TaskDescriptor **)&MediumPriProcList; break;
			case TaskPriorityLow: Chain=(T_TaskDescriptor **)&LowPriProcList; break;
			default: Chain=(T_TaskDescriptor **)&BkGroundProcList;
		}
		uint32_t SchedulerLock=RTK_SchedulerLock();
		if(*Chain){
			NewTask->Next=(*Chain)->Next;
			(*Chain)->Next=NewTask;
		}
		else{
			NewTask->Next=NewTask;
			*Chain=NewTask;
		}
		RTK_Unlock(SchedulerLock);
	}
	return NewTask;
}

/*					CreateNamedTask
	Purpose:
		Creates a named task and inserts it in the scheduler list selected by its priority.
		The new task starts in ready state.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK task creation function. It can be called while RTK is stopped to prepare the next
		SchedulerStart() cycle, or while RTK is running to create a task dynamically. The label is stored only when
		TASK_LABEL is not zero.
	Input:
		Task: task entry point.
		Label: packed task label used for diagnostics when TASK_LABEL is enabled.
		StkSize: task stack size, expressed in WORD units.
		Priority: task scheduling priority.
	Output:
		Pointer to the created task descriptor, or NULL if the descriptor and stack allocation fails.
*/
T_TaskDescriptor *CreateNamedTask(Func Task, T_Text Label, WORD StkSize, T_TaskPriority Priority){
	T_TaskDescriptor *NewTask;
	T_TaskDescriptor **Chain;
	size_t l=((StkSize<<1)+sizeof(T_RegisterFile)+sizeof(T_TaskDescriptor)+7)&0xFFFFFFF8;
	if((NewTask=(T_TaskDescriptor *)malloc(l))!=NULL){
		#if EVALUATE_FREE_STACK
			DWordFill(STACK_FILL_PATTERN, (DWORD *)NewTask, l>>2);
		#endif
		#if TASK_LABEL != 0
			NewTask->Label=Label;
		#endif
		#if STACK_GUARD
			NewTask->StackGuard=STACK_GUARD_PATTERN;
		#endif
		NewTask->PSP=(T_RegisterFile*)(((DWORD)NewTask+l-sizeof(T_RegisterFile))&0xFFFFFFF8);
		NewTask->PSP->LR=(DWORD)Terminate;
		NewTask->PSP->xPSR=0;
		NewTask->PSP->ExceptionNumber=0;
		NewTask->PSP->Tumb=1;
		NewTask->PSP->PC=(DWORD)Task |1; // Bit 0 selects the Thumb instruction set
		NewTask->TaskStatus.AsByte=0;
		NewTask->TaskPriority=Priority;
		NewTask->R14=0xFFFFFFFD;    // Return from interrupt with PSP
		InitTimer(&NewTask->Time);
		#if EXECUTION_CTR
			NewTask->TaskCtr=0;
		#endif
		#if IDLE_TIME
			NewTask->TimerCtrAtLastSched=TimerCtr;
		#endif
		switch(Priority){
			case TaskPriorityCritical: Chain=(T_TaskDescriptor **)&CriticalProcList; break;
			case TaskPriorityHi: Chain=(T_TaskDescriptor **)&HiPriProcList; break;
			case TaskPriorityMedium: Chain=(T_TaskDescriptor **)&MediumPriProcList; break;
			case TaskPriorityLow: Chain=(T_TaskDescriptor **)&LowPriProcList; break;
			default: Chain=(T_TaskDescriptor **)&BkGroundProcList;
		}
		uint32_t SchedulerLock=RTK_SchedulerLock();
		if(*Chain){
			NewTask->Next=(*Chain)->Next;
			(*Chain)->Next=NewTask;
		}
		else{
			NewTask->Next=NewTask;
			*Chain=NewTask;
		}
		RTK_Unlock(SchedulerLock);
	}
	return NewTask;
}

/*					CreateParTask
	Purpose:
		Creates a task with one instance parameter and inserts it in the scheduler list selected by its priority.
		The new task starts in ready state. The parameter is passed in R0 at first execution as a 32-bit integer; the
		task decides its effective type by casting it as needed.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK task creation function. It can be called while RTK is stopped to prepare the next
		SchedulerStart() cycle, or while RTK is running to create a task dynamically.
	Input:
		Task: task entry point accepting one 32-bit integer parameter.
		TaskParam: instance parameter copied into the initial R0 value. The task may cast it to the required type.
		StkSize: task stack size, expressed in WORD units.
		Priority: task scheduling priority.
	Output:
		Pointer to the created task descriptor, or NULL if the descriptor and stack allocation fails.
*/
T_TaskDescriptor *CreateParTask(FuncPar Task, DWORD TaskParam, WORD StkSize, T_TaskPriority Priority){
	T_TaskDescriptor *NewTask;
	T_TaskDescriptor **Chain;
	size_t l=((StkSize<<1)+sizeof(T_RegisterFile)+sizeof(T_TaskDescriptor)+7)&0xFFFFFFF8;
	if((NewTask=(T_TaskDescriptor *)malloc(l))!=NULL){
		#if EVALUATE_FREE_STACK
			DWordFill(STACK_FILL_PATTERN, (DWORD *)NewTask, l>>2);
		#endif
		#if TASK_LABEL== 16
			NewTask->Label=ALL_INVALID_16;  // ???
		#elif TASK_LABEL == 32
			NewTask->Label=ALL_INVALID_32;
		#elif TASK_LABEL == 64
			NewTask->Label=ALL_INVALID_64;
		#endif
		#if STACK_GUARD
			NewTask->StackGuard=STACK_GUARD_PATTERN;
		#endif
		NewTask->PSP=(T_RegisterFile*)(((DWORD)NewTask+l-sizeof(T_RegisterFile))&0xFFFFFFF8);
		NewTask->PSP->LR=(DWORD)Terminate;
		NewTask->PSP->xPSR=0;
		NewTask->PSP->ExceptionNumber=0;
		NewTask->PSP->Tumb=1;
		NewTask->PSP->PC=(DWORD)Task |1; // Bit 0 selects the Thumb instruction set
		NewTask->PSP->R0=TaskParam;
		NewTask->TaskStatus.AsByte=0;
		NewTask->TaskPriority=Priority;
		NewTask->R14=0xFFFFFFFD;    // Return from interrupt with PSP
		InitTimer(&NewTask->Time);
		#if EXECUTION_CTR
			NewTask->TaskCtr=0;
		#endif
		#if IDLE_TIME
			NewTask->TimerCtrAtLastSched=TimerCtr;
		#endif
		switch(Priority){
			case TaskPriorityCritical: Chain=(T_TaskDescriptor **)&CriticalProcList; break;
			case TaskPriorityHi: Chain=(T_TaskDescriptor **)&HiPriProcList; break;
			case TaskPriorityMedium: Chain=(T_TaskDescriptor **)&MediumPriProcList; break;
			case TaskPriorityLow: Chain=(T_TaskDescriptor **)&LowPriProcList; break;
			default: Chain=(T_TaskDescriptor **)&BkGroundProcList;
		}
		uint32_t SchedulerLock=RTK_SchedulerLock();
		if(*Chain){
			NewTask->Next=(*Chain)->Next;
			(*Chain)->Next=NewTask;
		}
		else{
			NewTask->Next=NewTask;
			*Chain=NewTask;
		}
		RTK_Unlock(SchedulerLock);
	}
	return NewTask;
}

/*					CreateNamedParTask
	Purpose:
		Creates a named task with one instance parameter and inserts it in the scheduler list selected by its priority.
		The new task starts in ready state. The parameter is passed in R0 at first execution as a 32-bit integer; the
		task decides its effective type by casting it as needed.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK task creation function. It can be called while RTK is stopped to prepare the next
		SchedulerStart() cycle, or while RTK is running to create a task dynamically. The label is stored only when
		TASK_LABEL is not zero.
	Input:
		Task: task entry point accepting one 32-bit integer parameter.
		TaskParam: instance parameter copied into the initial R0 value. The task may cast it to the required type.
		Label: packed task label used for diagnostics when TASK_LABEL is enabled.
		StkSize: task stack size, expressed in WORD units.
		Priority: task scheduling priority.
	Output:
		Pointer to the created task descriptor, or NULL if the descriptor and stack allocation fails.
*/
T_TaskDescriptor *CreateNamedParTask(FuncPar Task, DWORD TaskParam, T_Text Label, WORD StkSize, T_TaskPriority Priority){
	T_TaskDescriptor *NewTask;
	T_TaskDescriptor **Chain;
	size_t l=((StkSize<<1)+sizeof(T_TaskDescriptor)+sizeof(T_RegisterFile)+7)&0xFFFFFFF8;
	if((NewTask=(T_TaskDescriptor *)malloc(l))!=NULL){
		#if EVALUATE_FREE_STACK
			DWordFill(STACK_FILL_PATTERN, (DWORD *)NewTask, l>>2);
		#endif
		#if TASK_LABEL != 0
			NewTask->Label=Label;
		#endif
		#if STACK_GUARD
			NewTask->StackGuard=STACK_GUARD_PATTERN;
		#endif
		NewTask->PSP=(T_RegisterFile*)(((DWORD)NewTask+l-sizeof(T_RegisterFile))&0xFFFFFFF8);
		NewTask->PSP->LR=(DWORD)Terminate;
		NewTask->PSP->xPSR=0;
		NewTask->PSP->ExceptionNumber=0;
		NewTask->PSP->Tumb=1;
		NewTask->PSP->PC=(DWORD)Task |1; // Bit 0 selects the Thumb instruction set
		NewTask->PSP->R0=TaskParam;
		NewTask->TaskStatus.AsByte=0;
		NewTask->TaskPriority=Priority;
		NewTask->R14=0xFFFFFFFD;    // Return from interrupt with PSP
		InitTimer(&NewTask->Time);
		#if EXECUTION_CTR
			NewTask->TaskCtr=0;
		#endif
		#if IDLE_TIME
			NewTask->TimerCtrAtLastSched=TimerCtr;
		#endif
		switch(Priority){
			case TaskPriorityCritical: Chain=(T_TaskDescriptor **)&CriticalProcList; break;
			case TaskPriorityHi: Chain=(T_TaskDescriptor **)&HiPriProcList; break;
			case TaskPriorityMedium: Chain=(T_TaskDescriptor **)&MediumPriProcList; break;
			case TaskPriorityLow: Chain=(T_TaskDescriptor **)&LowPriProcList; break;
			default: Chain=(T_TaskDescriptor **)&BkGroundProcList;
		}
		uint32_t SchedulerLock=RTK_SchedulerLock();
		if(*Chain){
			NewTask->Next=(*Chain)->Next;
			(*Chain)->Next=NewTask;
		}
		else{
			NewTask->Next=NewTask;
			*Chain=NewTask;
		}
		RTK_Unlock(SchedulerLock);
	}
	return NewTask;
}

/*					CreateNamedMultiParsTask
	Purpose:
		Creates a named task with four instance parameters and inserts it in the scheduler list selected by its priority. The new 
		task starts in ready state. Parameters are passed in R0, R1, R2 and R3 at first execution as 32-bit integers; the task 
		decides their effective types by casting them as needed.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK task creation function. It can be called while RTK is stopped to prepare the next SchedulerStart() 
		cycle, or while RTK is running to create a task dynamically. This implementation is specific to	the ARM Cortex-M call model 
		and cannot be generalized to other architectures without review. The label is stored only when TASK_LABEL is not zero.
	Input:
		Task: task entry point accepting four 32-bit integer parameters.
		Label: packed task label used for diagnostics when TASK_LABEL is enabled.
		StkSize: task stack size, expressed in WORD units.
		Priority: task scheduling priority.
		P0: first instance parameter, copied into the initial R0 value.
		P1: second instance parameter, copied into the initial R1 value.
		P2: third instance parameter, copied into the initial R2 value.
		P3: fourth instance parameter, copied into the initial R3 value.
	Output:
		Pointer to the created task descriptor, or NULL if the descriptor and stack allocation fails.
*/
T_TaskDescriptor *CreateNamedMultiParsTask(MultiFuncPar Task, T_Text Label, WORD StkSize, T_TaskPriority Priority, DWORD P0, DWORD P1, DWORD P2, DWORD P3){
	T_TaskDescriptor *NewTask;
	T_TaskDescriptor **Chain;
	size_t l=((StkSize<<1)+sizeof(T_TaskDescriptor)+sizeof(T_RegisterFile)+7)&0xFFFFFFF8;
	if((NewTask=(T_TaskDescriptor *)malloc(l))!=NULL){
		#if EVALUATE_FREE_STACK
			DWordFill(STACK_FILL_PATTERN, (DWORD *)NewTask, l>>2);
		#endif
		#if TASK_LABEL != 0
			NewTask->Label=Label;
		#endif
		#if STACK_GUARD
			NewTask->StackGuard=STACK_GUARD_PATTERN;
		#endif
		NewTask->PSP=(T_RegisterFile*)(((DWORD)NewTask+l-sizeof(T_RegisterFile))&0xFFFFFFF8);
		NewTask->PSP->LR=(DWORD)Terminate;
		NewTask->PSP->xPSR=0;
		NewTask->PSP->ExceptionNumber=0;
		NewTask->PSP->Tumb=1;
		NewTask->PSP->PC=(DWORD)Task |1; // Bit 0 selects the Thumb instruction set
		NewTask->PSP->R0=P0;
		NewTask->PSP->R1=P1;
		NewTask->PSP->R2=P2;
		NewTask->PSP->R3=P3;
		NewTask->TaskStatus.AsByte=0;
		NewTask->TaskPriority=Priority;
		NewTask->R14=0xFFFFFFFD;    // Return from interrupt with PSP
		InitTimer(&NewTask->Time);
		#if EXECUTION_CTR
			NewTask->TaskCtr=0;
		#endif
		#if IDLE_TIME
			NewTask->TimerCtrAtLastSched=TimerCtr;
		#endif
		switch(Priority){
			case TaskPriorityCritical: Chain=(T_TaskDescriptor **)&CriticalProcList; break;
			case TaskPriorityHi: Chain=(T_TaskDescriptor **)&HiPriProcList; break;
			case TaskPriorityMedium: Chain=(T_TaskDescriptor **)&MediumPriProcList; break;
			case TaskPriorityLow: Chain=(T_TaskDescriptor **)&LowPriProcList; break;
			default: Chain=(T_TaskDescriptor **)&BkGroundProcList;
		}
		uint32_t SchedulerLock=RTK_SchedulerLock();
		if(*Chain){
			NewTask->Next=(*Chain)->Next;
			(*Chain)->Next=NewTask;
		}
		else{
			NewTask->Next=NewTask;
			*Chain=NewTask;
		}
		RTK_Unlock(SchedulerLock);
	}
	return NewTask;
}

/*					KillTask
	Purpose:
		Removes a task from its scheduler list, disarms its wait timer and releases its descriptor and stack memory.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK task deletion function. It must be called with a pointer to an existing task descriptor. If the pointer 
		refers to a task that has already terminated, the consequences are unpredictable. The function must not be called by the 
		task that is being deleted; a task must call Terminate() or return from its task function to delete itself. The function 
		locks scheduler list updates while unlinking the task.
	Input:
		TaskToDelete: pointer to an existing task descriptor. Passing NULL has no effect.
	Output:
		None. If TaskToDelete is not NULL, the task is removed from its priority list and its memory is freed.
	Notes:
		In the next review, evaluate whether KillTask() should check that TaskToDelete still belongs to a scheduler list.
*/
void KillTask(T_TaskDescriptor *TaskToDelete){
	T_TaskDescriptor *Cnt;
	if(TaskToDelete){
		uint32_t SchedulerLock=RTK_SchedulerLock();
		// For each priority level, first check whether the task is the first element in the circular task list and, if so, move the
		// list pointer to the next element. Then check whether it was the only task: if the first element still points to it, set 
		// the list pointer to NULL because no other tasks with the same priority remain.
		if(TaskToDelete==BkGroundProcList)
			if((BkGroundProcList=TaskToDelete->Next)==TaskToDelete)
				BkGroundProcList=NULL;
		if(TaskToDelete==LowPriProcList)
			if((LowPriProcList=TaskToDelete->Next)==TaskToDelete)
				LowPriProcList=NULL;
		if(TaskToDelete==MediumPriProcList)
			if((MediumPriProcList=TaskToDelete->Next)==TaskToDelete)
				MediumPriProcList=NULL;
		if(TaskToDelete==HiPriProcList)
			if((HiPriProcList=TaskToDelete->Next)==TaskToDelete)
				HiPriProcList=NULL;
		if(TaskToDelete==CriticalProcList)
			if((CriticalProcList=TaskToDelete->Next)==TaskToDelete)
				CriticalProcList=NULL;
		// Find the element preceding the task to delete and make it point to the following element.
		// If this is the only task in the list, it continues to point to itself without causing any problems.
		Cnt=TaskToDelete;
		while(Cnt->Next!=TaskToDelete)
			Cnt=Cnt->Next;
		Cnt->Next=TaskToDelete->Next;
		// Finally, disarm any associated timer and delete the task.
		DisarmaTimer(&(TaskToDelete->Time));
		free(TaskToDelete);
		RTK_Unlock(SchedulerLock);
	}
}

/*					Terminate
	Purpose:
		Requests termination of the currently running task.
		The task is destroyed by FirstToRun() during the following scheduling pass.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK task termination function. Its address is also installed as the return address of each new task, so a 
		task is automatically terminated if its entry function returns. The function clears BASEPRI before requesting PendSV, so a 
		task that terminates while the scheduler is locked cannot leave PendSV masked.
	Input:
		None.
	Output:
		None. DeleteCurrentTask is set and PendSV is requested.
*/
void Terminate(){
	DeleteCurrentTask=true;
	__set_BASEPRI(0U);
	__DSB();
	__ISB();
	SCHEDULE_HIGHTEST
}

/*					ChangeTaskPriority
	Purpose:
		Changes the scheduling priority of an existing task.
		The task is removed from its current priority list and inserted in the list selected by NewPriority.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is a public RTK task management function. It must be called with a pointer to an existing task descriptor. If the 
		pointer refers to a task that has already terminated, the consequences are unpredictable. The function locks scheduler list 
		updates while moving the task between priority lists.
	Input:
		Task: pointer to the task descriptor whose priority must be changed. Passing NULL is rejected.
		NewPriority: new scheduling priority for the task.
	Output:
		true if the task priority has been changed.
		false if Task is NULL.
	Notes:
		In the next review, evaluate whether ChangeTaskPriority() should check that Task still belongs to a scheduler
		list before moving it.
*/
bool ChangeTaskPriority(T_TaskDescriptor *Task, T_TaskPriority NewPriority){
	T_TaskDescriptor *Cnt;
	if(Task){
		uint32_t SchedulerLock=RTK_SchedulerLock();
		if(Task==BkGroundProcList) if((BkGroundProcList=Task->Next)==Task) BkGroundProcList=NULL;
		if(Task==LowPriProcList) if((LowPriProcList=Task->Next)==Task) LowPriProcList=NULL;
		if(Task==MediumPriProcList) if((MediumPriProcList=Task->Next)==Task) MediumPriProcList=NULL;
		if(Task==HiPriProcList) if((HiPriProcList=Task->Next)==Task) HiPriProcList=NULL;
		if(Task==CriticalProcList) if((CriticalProcList=Task->Next)==Task) CriticalProcList=NULL;
		Cnt=Task;
		while(Cnt->Next!=Task) Cnt=Cnt->Next;
		Cnt->Next=Task->Next;
		Task->TaskPriority=NewPriority;
		T_TaskDescriptor **Chain;
		switch(NewPriority){
			case TaskPriorityCritical: Chain=(T_TaskDescriptor **)&CriticalProcList; break;
			case TaskPriorityHi: Chain=(T_TaskDescriptor **)&HiPriProcList; break;
			case TaskPriorityMedium: Chain=(T_TaskDescriptor **)&MediumPriProcList; break;
			case TaskPriorityLow: Chain=(T_TaskDescriptor **)&LowPriProcList; break;
			default: Chain=(T_TaskDescriptor **)&BkGroundProcList;
		}
		if(*Chain){
			Task->Next=(*Chain)->Next;
			(*Chain)->Next=Task;
		}
		else{
			Task->Next=Task;
			*Chain=Task;
		}
		RTK_Unlock(SchedulerLock);
		return true;
	}
	return false;
}

/*					SysTick_Handler
	Purpose:
		Handles the RTK system tick.
		It updates software timers, processes tic objects, updates optional counters and requests scheduling when needed.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This is the ARM Cortex-M SysTick interrupt handler used by RTK. It runs in interrupt context and must remain short.
		It does not perform the context switch directly; when scheduling is required it requests PendSV.
	Input:
		None.
	Output:
		None. uwTick, software timers, optional diagnostics and scheduler state may be updated. PendSV may be requested.
*/
extern volatile uint32_t uwTick;	// Compatibility patch for ST libraries that use it for timeouts
__attribute__((used)) void SysTick_Handler(void){
	OUT_SYSTIC(1);							// Output the SysTick state on a digital pin
	uwTick++;
	TimerTic();          					// Process timers
	#if TIC_OBJs
		TicObjectProcess(); 				// Process scheduler tic objects
	#endif
	#if _EXECUTION_CTR
		CurrentTaskPtr->TaskCtr++;
	#endif
	if(TauCtr) TauCtr--;					// Decrement TauCtr only if it has not already expired
 	if(CurrentTaskPtr->TaskPriority){		// Schedule only when a critical-priority task is not running
		if(TauCtr==0){						// Tau expired?
			TauCtr=TicPerTau;				// Reload the counter
			SchedulazioneCompleta=true;		// Request a complete scheduling pass
		}
		else								// Or when the idle task is running
			if(CurrentTaskPtr->TaskPriority==TaskPriorityIdle)
				SchedulazioneCompleta=true;
		*SCB_ICSR=0x10000000;				// Request scheduling
	}
	OUT_SYSTIC(0);							// Output the SysTick state on a digital pin
}
