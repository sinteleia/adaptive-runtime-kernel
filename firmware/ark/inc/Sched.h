/*						ARK Project - Adaptive Runtime Kernel

	Module:
		Sched.h

	Purpose:
		Scheduler public types, task descriptor layout and low-level RTK scheduler services.

	Description:
		This header defines scheduler wait states, task priorities, task descriptor structures,
		scheduler globals and inline interrupt-masking helpers used by RTK internals and selected
		application-facing services. The task descriptor layout is shared with assembly code through
		the offsets defined in SchedAsmOffsets.h.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK scheduler headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __Sched_h
	#define __Sched_h

	#include "RTK_Config.h"
	#include "type.h"
	#include "TimerTic.h"
	#include "BinQue.h"
	#include "FreeQue.h"
	#include "Sem.h"
	#include "CountingSem.h"
	#include "MyIntrinsics.h"
	#include "SchedAsmOffsets.h"
	#include "stddef.h"

	extern uint32_t RTK_SchedulerBasepri;
	extern uint32_t RTK_SysTicBasepri;

	/*							T_WaitingType
		E' indispensabile che le attese con TO abbiano codice uguale alle relative senza TO + 0x80.
	*/
	typedef enum T_WaitingType{
		WaitingForNone=0,
		WaitingForTime=1,
		WaitingForever=2,
		WaitingForSemaphore=3,
		WaitingForCountingSem=4,
		WaitingForFlag=5,
		WaitingForNotFlag=6,
		WaitingForQueGet=7,
		WaitingForBynaryLenQuePut=8,
		WaitingForFreeLenQuePut=9,
		WaitingForQueEmpty=10,
		WaitingForBit=11,
		WaitingForNotBit=12,
		WaitingForWordBit=13,
		WaitingForWordNotBit=14,
		WaitingForDWordBit=15,
		WaitingForDWordNotBit=16,
		WaitingForCanQueSpace=17,
		WaitingForCanQueMessage=18,
		InvalidWait=19,
		WaitingForeverTO=0x82,			// 2
		WaitingForSemaphoreTO,			// 3
		WaitingForCountingSemTO,		// 4
		WaitingForFlagTO,				// 5
		WaitingForNotFlagTO,			// 6
		WaitingForQueGetTO,				// 7
		WaitingForBynaryLenQuePutTO,	// 8
		WaitingForFreeLenQuePutTO,		// 9
		WaitingForQueEmptyTO,			// 10
		WaitingForBitTO,				// 11
		WaitingForNotBitTO,				// 12
		WaitingForWordBitTO,			// 13
		WaitingForWordNotBitTO,			// 14
		WaitingForDWordBitTO,			// 15
		WaitingForDWordNotBitTO,		// 16
		WaitingForCanQueSpaceTO,		// 17
		WaitingForCanQueMessageTO		// 18
	}T_WaitingType;

	typedef struct T_TaskStatus{
		BYTE WaitingFor			:7;			// Attende lo scadere di un tempo
		BYTE WaitingWithTimeOut	:1;			// Abilita l'uscita per time out dall'attesa
	}T_TaskStatus;

	typedef enum T_TaskPriority{
		TaskPriorityCritical=0,				// Non  vengono interrotte dal TIC
		TaskPriorityHi,						// Se ce ne sono girano solo loro
		TaskPriorityMedium,					// Girano ad ogni quanta se la CPU   libera
		TaskPriorityLow,					// Ne viene schedulata una ogni giro di Medium
		TaskPriorityBackGround,				// Girano se la CPU non ha niente di meglio da fare
		TaskPriorityIdle,					// Gira quando non ci sono task ready.
		InvalidTaskPriority=0xFF 			// Priorit� non valida
	}T_TaskPriority;

	/*			T_RegisterFile
			Registri del processore che vengono salvati automaticamente dall'interrupt.
	*/
	typedef struct T_RegisterFile{
		DWORD R0;
		DWORD R1;
		DWORD R2;
		DWORD R3;
		DWORD R12;
		DWORD LR;
		DWORD PC;
		union{
			DWORD xPSR;
			struct{
				DWORD ExceptionNumber: 9;
				DWORD res: 1;
				DWORD ICI_IT: 6;
				DWORD res1: 8;
				DWORD Tumb: 1;       // Deve essere 1;
				DWORD ICI_IT1: 2;
				DWORD Q: 1;
				DWORD V: 1;
				WORD C: 1;
				DWORD Z: 1;
				DWORD N: 1;
			};
		};
		float S0_15[16];
		DWORD FPSCR;
		DWORD	Aligner;
	}T_RegisterFile;

	/*			S_TaskDescriptor

		DWORD		|		DWORD
		PSP			|		R4
		R5			|		R6
		R7			|		R8
		R9			|		R10
		R11			|		R14
		S16         |       S17
		Next		|	Object to wait
		Time
		Param+TaskStatus+TaskPriority	|
	*/

	#if TASK_LABEL == 16
		typedef WORD T_Text;
		#define RTK_Pack Pack16
	#elif TASK_LABEL == 32
		typedef DWORD T_Text;
		#define RTK_Pack Pack32
	#elif TASK_LABEL == 64
		typedef QWORD T_Text;
		#define RTK_Pack Pack64
	#endif

	struct S_TaskDescriptor{
		T_RegisterFile *PSP;
		DWORD R4;
		DWORD R5;
		DWORD R6;
		DWORD R7;
		DWORD R8;
		DWORD R9;
		DWORD R10;
		DWORD R11;
		DWORD R14;
		float S16_31[16];
		struct S_TaskDescriptor *Next; 			// Puntatore alla prossima task della lista
		union{
			TQueHeader *Q;
			TBinaryLenQueHeader *BQ;
			TFreeLenQueHeader *FQ;
			volatile BYTE *C;
			volatile WORD *W;
			volatile DWORD *DW;
			volatile Semaphore *S;
			volatile T_CountingSem *CS;
			volatile Flag *F;
			// T_CanQue *CQ;
		}ObjectToWait;							// Puntatore all'oggetto da attendere
		T_Timer Time;							// Timer per la sospensione a tempo e per i time out
		union {
			DWORD DW_Param;
			WORD W_Param;
			BYTE B_Param;
		}Param;									// Eventuali parametri per l'attesa
		volatile union {
			T_TaskStatus AsBit;
			BYTE AsByte;
		}TaskStatus;
		BYTE TaskPriority;
		#if EXECUTION_CTR
			WORD TaskCtr;						// Numero di volte che la task è stata schedulata
		#endif
		#if TASK_LABEL != 0
			T_Text Label;						// Nome (3 char) della task
		#endif
		#if LOCAL_LAST_ERROR					// Last error della task. Usato dalle funzioni che possono fallire (come ad
			WORD LocalLastError;				// esempio quelle del file system), per esplicitare l'errore
		#endif
		#if IDLE_TIME
			DWORD TimerCtrAtLastSched;			// Valore del timer ctr l'ultima volta che la task è stata schedulata
		#endif
		#if CALLER_ADDRESS
			void *WaitCallerAddress;
		#endif
		#if STACK_GUARD
			DWORD StackGuard;					// Guard pattern checked by PendSV_Handler before saving the task context.
		#endif
	};
	typedef struct S_TaskDescriptor T_TaskDescriptor;

	#if STACK_GUARD
		/*
			Assembly code uses the StackGuard offset defined in SchedAsmOffsets.h.
			Make the build fail if the compiler-generated C/C++ layout no longer matches it.
		*/
		#ifdef __cplusplus
			static_assert(
				offsetof(T_TaskDescriptor, StackGuard) == TASK_STACK_GUARD_OFFSET,
				"T_TaskDescriptor.StackGuard offset mismatch"
			);
		#else
			_Static_assert(
				offsetof(T_TaskDescriptor, StackGuard) == TASK_STACK_GUARD_OFFSET,
				"T_TaskDescriptor.StackGuard offset mismatch"
			);
		#endif
	#endif

	/*				StatoHandler
		Possibili stati di un handler a task.
	*/
	typedef enum StatoHandler{
		Reserved,
		Used,
		Free,
		Unavailable
	}StatoHandler;

	extern volatile bool KernelRunning;			// Flag che indica il fatto che lo scheduler sia gi� stato startato

	extern volatile T_TaskDescriptor *CriticalProcList;
	extern volatile T_TaskDescriptor *HiPriProcList;
	extern volatile T_TaskDescriptor *MediumPriProcList;
	extern volatile T_TaskDescriptor *LowPriProcList;
	extern volatile T_TaskDescriptor *BkGroundProcList;
	extern volatile T_TaskDescriptor *CurrentTaskPtr;
	extern bool SchedulazioneCompleta;

	#ifdef __cplusplus
		extern "C" {
	#endif

	/*
		IdleTask

		Purpose:
			Provide the default idle task executed when no user task is ready.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Pass this routine to SchedulerInit() when the application does not require a custom idle task.
		Input:
			None.
		Output:
			Does not return.
		Notes:
			The application may replace the weak implementation by defining a non-weak IdleTask().
	*/
	void IdleTask(void);

	/*						SchedulerInit
		Purpose:
			Initialize the scheduler configuration and internal lists.
			Call at least once before the first SchedulerStart() invocation.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call only while RTK is stopped;
		Input:
			IdleTask - Idle task entry point.
			ParTicPerTau - Number of system ticks used to generate one scheduler tau.
			ParMediumForLow - Medium-to-low scheduling ratio before the scan order is inverted.
			MaxSchedRoutines - Maximum scheduler tic routines when TIC_OBJs is enabled.
			MaxISR_Routines - Maximum ISR tic routines when TIC_OBJs is enabled.
		Output:
			true when the scheduler configuration has been initialized.
	*/
	bool SchedulerInit(Func IdleTask, BYTE ParTicPerTau, BYTE ParMediumForLow
		#if TIC_OBJs
			,WORD MaxSchedRoutines, WORD MaxISR_Routines
		 #endif
	);

	/*						CreateTask
		Purpose:
			Create a task, allocate its descriptor and stack, and insert it in the selected scheduler list.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call while RTK is stopped or from task context while RTK is running.
		Input:
			Task - Task entry point.
			StkSize - Requested stack size in WORD units.
			Priority - Scheduler priority assigned to the task.
		Output:
			Pointer to the new task descriptor, or NULL when allocation fails.
	*/
	T_TaskDescriptor *CreateTask(Func Task, WORD StkSize, T_TaskPriority Priority);

	/*						CreateNamedTask
		Purpose:
			Create a named task and insert it in the scheduler list selected by its priority.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call while RTK is stopped or from task context while RTK is running.
		Input:
			Task - Task entry point.
			Label - Packed task label used by diagnostics.
			StkSize - Requested stack size in WORD units.
			Priority - Scheduler priority assigned to the task.
		Output:
			Pointer to the new task descriptor, or NULL when allocation fails.
	*/
	T_TaskDescriptor *CreateNamedTask(Func Task, T_Text Label, WORD StkSize, T_TaskPriority Priority);

	/*						CreateParTask
		Purpose:
			Create a task with one parameter and insert it in the scheduler list selected by its priority.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call while RTK is stopped or from task context while RTK is running.
		Input:
			Task - Task entry point accepting one DWORD parameter.
			TaskParam - Value placed in R0 for the first task invocation.
			StkSize - Requested stack size in WORD units.
			Priority - Scheduler priority assigned to the task.
		Output:
			Pointer to the new task descriptor, or NULL when allocation fails.
	*/
	T_TaskDescriptor *CreateParTask(FuncPar Task, DWORD TaskParam, WORD StkSize, T_TaskPriority Priority);

	/*						CreateNamedParTask
		Purpose:
			Create a named task with one parameter and insert it in the selected scheduler list.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call while RTK is stopped or from task context while RTK is running.
		Input:
			Task - Task entry point accepting one DWORD parameter.
			TaskParam - Value placed in R0 for the first task invocation.
			Label - Packed task label used by diagnostics.
			StkSize - Requested stack size in WORD units.
			Priority - Scheduler priority assigned to the task.
		Output:
			Pointer to the new task descriptor, or NULL when allocation fails.
	*/
	T_TaskDescriptor *CreateNamedParTask(FuncPar Task, DWORD TaskParam, T_Text Label, WORD StkSize, T_TaskPriority Priority);

	/*						CreateNamedMultiParsTask
		Purpose:
			Create a named task with four parameters and insert it in the selected scheduler list.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call while RTK is stopped or from task context while RTK is running on the supported ARM Cortex-M port.
		Input:
			Task - Task entry point accepting four DWORD parameters.
			Label - Packed task label used by diagnostics.
			StkSize - Requested stack size in WORD units.
			Priority - Scheduler priority assigned to the task.
			P0, P1, P2, P3 - Values placed in R0, R1, R2 and R3 for the first task invocation.
		Output:
			Pointer to the new task descriptor, or NULL when allocation fails.
	*/
	T_TaskDescriptor *CreateNamedMultiParsTask(MultiFuncPar Task, T_Text Label, WORD StkSize,
	                                           T_TaskPriority Priority, DWORD P0, DWORD P1,
	                                           DWORD P2, DWORD P3);

	/*						SchedulerStart
		Purpose:
			Start one scheduler execution cycle and return only when all user tasks have terminated.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call while RTK is stopped after SchedulerInit() has stored a valid idle task entry point.
		Input:
			None.
		Output:
			true after a regular scheduler cycle, false when scheduler startup fails.
	*/
	bool SchedulerStart(void);

	/*						Terminate
		Purpose:
			Request termination and deferred destruction of the currently running task.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call from task context; task entry points also use this routine as their return address.
		Input:
			None.
		Output:
			Does not return to the terminating task.
		Notes:
			The descriptor and stack are released by the scheduler during the following context switch.
	*/
	void Terminate(void);

	/*						KillTask
		Purpose:
			Remove an existing task from its scheduler list and release its descriptor and stack.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call with a valid descriptor belonging to a live task other than the current task.
		Input:
			TaskToDelete - Descriptor of the task to remove; NULL has no effect.
		Output:
			None.
		Notes:
			Never use this routine to delete the currently running task; use Terminate() instead.
	*/
	void KillTask(T_TaskDescriptor *TaskToDelete);

	/*						ChangeTaskPriority
		Purpose:
			Move an existing task to the scheduler list selected by a new priority.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call with a valid descriptor belonging to a live scheduler task.
		Input:
			Task - Descriptor of the task whose priority must change.
			NewPriority - New scheduler priority.
		Output:
			true when the priority is changed, false when Task is NULL.
	*/
	bool ChangeTaskPriority(T_TaskDescriptor *Task, T_TaskPriority NewPriority);

	/*						PresetTaskTimer
		Purpose:
			Set or update the timer associated with the currently running task.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Used from task context, normally by wait primitives that support timeouts.
		Input:
			Time - Relative timeout in system ticks; zero preserves the current task timer.
		Output:
			None.
		Notes:
			Preserving the timer allows one global timeout to span multiple wait operations.
	*/
	inline void PresetTaskTimer(DWORD Time){
		if(Time)
			SetTimer(&(((T_TaskDescriptor *)CurrentTaskPtr)->Time), Time);
	}

	/*						RTK_SchedulerLock
		Purpose:
			Mask scheduler context switches without weakening an existing BASEPRI mask.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Use around short critical sections that must prevent PendSV scheduling.
		Input:
			None.
		Output:
			Previous BASEPRI value to pass to RTK_Unlock().
		Notes:
			BASEPRI_MAX changes the mask only when the requested scheduler threshold is more restrictive.
	*/
	static inline uint32_t RTK_SchedulerLock(void){
		uint32_t old_basepri = __get_BASEPRI();
		__set_BASEPRI_MAX(RTK_SchedulerBasepri);
		__DSB();
		__ISB();
		return old_basepri;
	}

	/*						RTK_SysTicLock
		Purpose:
			Mask the RTK system tick and scheduler without weakening an existing BASEPRI mask.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Use around short critical sections that must exclude SysTick and PendSV processing.
		Input:
			None.
		Output:
			Previous BASEPRI value to pass to RTK_Unlock().
		Notes:
			BASEPRI_MAX changes the mask only when the requested system-tick threshold is more restrictive.
	*/
	static inline uint32_t RTK_SysTicLock(void){
		uint32_t old_basepri = __get_BASEPRI();
		__set_BASEPRI_MAX(RTK_SysTicBasepri);
		__DSB();
		__ISB();
		return old_basepri;
	}

	/*						RTK_Unlock
		Purpose:
			Restore the BASEPRI value saved by an RTK lock routine.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Call at the end of a critical section entered through RTK_SchedulerLock() or RTK_SysTicLock().
		Input:
			old_basepri - Exact BASEPRI value returned by the matching lock call.
		Output:
			None.
		Notes:
			A pending PendSV may execute immediately after the previous interrupt mask is restored.
	*/
	static inline void RTK_Unlock(uint32_t old_basepri){
		__set_BASEPRI(old_basepri);
		__DSB();
		__ISB();
	}

	#ifdef __cplusplus
		}
	#endif

	#define SCB_ICSR ((volatile DWORD*)0xE000ED04)
	#define SCHEDULE_HIGHTEST {*SCB_ICSR=0x10000000; __DSB(); __ISB();}
	#define SCHEDULE {SchedulazioneCompleta=true; SCHEDULE_HIGHTEST}

	#define STACK_FILL_PATTERN 0x089ABCDEFUL

#endif

