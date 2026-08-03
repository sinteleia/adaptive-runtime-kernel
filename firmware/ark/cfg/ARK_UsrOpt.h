/*
						ARK Project - Adaptive Runtime Kernel

	Module:
		ARK_UsrOpt.h

	Purpose:
		Project-specific compile-time option overrides for ARK/RTK and the ARK memory manager.

	Description:
		This file is included, when present, before the ARK default configuration values in RTK_Config.h
		and MM.cfg. Define options here only when the application must override the ARK defaults.
		Unless expressly specified, every option can be defined as equal to 1 or 0 to override the
		default configuration.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Created for ARK 1.0 as the project-level user configuration override file.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/

// 	-------------- RTK wait options ----------------

// WAIT_FOR_QUE_EMPTY enables wait operations for empty queues.
// WAIT_FOR_QUE_GET enables wait operations for queue data availability.
// WAIT_FOR_BYNARY_LEN_QUE_PUT enables wait operations for binary length queue put space.
// WAIT_FOR_FREE_LEN_QUE_PUT enables wait operations for free length queue put space.
// WAIT_FOR_FLAG enables wait operations for a flag becoming true.
// WAIT_FOR_NOT_FLAG enables wait operations for a flag becoming false.
// WAIT_FOR_BIT enables wait operations for a byte bit becoming set.
// WAIT_FOR_NOT_BIT enables wait operations for a byte bit becoming clear.
// WAIT_FOR_WORD_BIT enables wait operations for a word bit becoming set.
// WAIT_FOR_NOT_WORD_BIT enables wait operations for a word bit becoming clear.
// WAIT_FOR_DWORD_BIT enables wait operations for a dword bit becoming set.
// WAIT_FOR_NOT_DWORD_BIT enables wait operations for a dword bit becoming clear.
// WAIT_FOR_SEM enables wait operations for binary semaphores.
// WAIT_FOR_COUNTING_SEM enables wait operations for counting semaphores.
// WAIT_FOR_ALMENO_UN_BIT enables wait operations for any byte mask bit becoming set.
// WAIT_FOR_NESSUN_BIT enables wait operations for all byte mask bits becoming clear.
// WAIT_FOR_ALMENO_UN_WORD_BIT enables wait operations for any word mask bit becoming set.
// WAIT_FOR_NESSUN_WORD_BIT enables wait operations for all word mask bits becoming clear.
// WAIT_FOR_ALMENO_UN_DWORD_BIT enables wait operations for any dword mask bit becoming set.
// WAIT_FOR_NESSUN_DWORD_BIT enables wait operations for all dword mask bits becoming clear.

// --------------- RTK diagnostic and scheduler options -----------------

// EXECUTION_CTR enables per-task execution counters.
// TASK_LABEL selects task label storage size; **** supported values are 0, 16, 32 and 64 ****.
// LOCAL_LAST_ERROR enables per-task last-error storage.
// IDLE_TIME enables task timing fields used for idle/time diagnostics.
// EVALUATE_FREE_STACK enables stack fill patterns used to estimate free stack space.
// TIC_OBJs enables tic and scheduler callback objects.
// CONSTANT_SCHEDULING_TIME keeps tic object processing time bounded per scheduler pass.
// CALLER_ADDRESS stores caller addresses in wait diagnostics where supported.
// SCHEDULE_DIAG enables scheduler resume-cause diagnostics.
// OUT_SYSTIC(x) **** can be defined as a macro to out a digital value x during SysTick activity ****.
// OUT_PENDVS(x) **** can be defined as a macro to out a digital value x during PendSV activity ****.
// Example C compiler option used by the STM32H743 test board:
// -DOUT_SYSTIC(VALUE)=(*((volatile unsigned int *)0x58020418)=((((VALUE)&1U)<<5)|(((((VALUE)&1U)^1U)<<21))))
// Example assembler compiler option used by the STM32H743 test board:
// -DOUT_PENDVS(VALUE)=LDR R12,=0x58020418; LDR R2,=((((VALUE)&1)<<4)|(((((VALUE)&1)^1)<<20))); STR R2,[R12]
// STACK_GUARD enables stack overflow checking for the task being scheduled out.

// --------------- Memory manager protection options -----------------

// Exactly one malloc protection mode must be enabled.
// MALLOC_INTERRUPT_PROTECT protects malloc/free by globally disabling interrupts.
// MALLOC_SCHEDULER_PROTECT protects malloc/free by masking scheduler activity.
// MALLOC_SEMAPHORE_PROTECT protects malloc/free with a semaphore.


// --------------- Memory manager diagnostic options -----------------

// BLOCK_COUNTER enables allocation/free block counting diagnostics.
// MALLOC_TEST enables heap consistency checks around malloc/free.
// MALLOC_GUARD adds a guard word at the end of allocated blocks.

// --------------- Timer diagnostic options -----------------
// TIMER_INTERRUPT_PROTECT if defined at 0, use scheduler interrupt lock to protect from concurrency. This
// protection is INSUFFICENT if the tic priority differs from the scheduler priority because the tic call
// the timer call the TimerTic routine that operate on the timer list.
// TIMER_NUMBER_CHECK enable check on the number of active timers in the que.

#define TIMER_INTERRUPT_PROTECT 0
