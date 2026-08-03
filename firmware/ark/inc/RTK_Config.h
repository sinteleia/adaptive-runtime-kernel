/*						ARK Project - Adaptive Runtime Kernel

	Module:
		RTK_Config.h

	Purpose:
		Default compile-time configuration options for the RTK kernel.

	Description:
		This header provides RTK default configuration values when the project-specific ARK_UsrOpt.h
		file is not available or does not define a specific option. It centralizes scheduler, wait,
		diagnostic, stack, timer and instrumentation switches used by the RTK build.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Created for ARK 1.0. Earlier RTK versions used a project-local user configuration file.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __RTK_CONFIG_H
	#define __RTK_CONFIG_H

	#if defined(__has_include)
		#if __has_include("ARK_UsrOpt.h")
			#include "ARK_UsrOpt.h"
			#define RTK_USR_OPT_PRESENT 1
		#else
			#define RTK_USR_OPT_PRESENT 0
		#endif
	#else
		#define RTK_USR_OPT_PRESENT 0
	#endif

	#if !RTK_USR_OPT_PRESENT
		#warning "ARK_UsrOpt.h not found; using RTK_Config.h default options"
	#endif

	// Waiting cause enabled
	#ifndef WAIT_FOR_QUE_EMPTY
		#define WAIT_FOR_QUE_EMPTY 1
	#endif
	#ifndef WAIT_FOR_QUE_GET
		#define WAIT_FOR_QUE_GET 1
	#endif
	#ifndef WAIT_FOR_BYNARY_LEN_QUE_PUT
		#define WAIT_FOR_BYNARY_LEN_QUE_PUT 1
	#endif
	#ifndef WAIT_FOR_FREE_LEN_QUE_PUT
		#define WAIT_FOR_FREE_LEN_QUE_PUT 0
	#endif
	#ifndef WAIT_FOR_FLAG
		#define WAIT_FOR_FLAG 1
	#endif
	#ifndef WAIT_FOR_NOT_FLAG
		#define WAIT_FOR_NOT_FLAG 1
	#endif
	#ifndef WAIT_FOR_BIT
		#define WAIT_FOR_BIT 1
	#endif
	#ifndef WAIT_FOR_NOT_BIT
		#define WAIT_FOR_NOT_BIT 1
	#endif
	#ifndef WAIT_FOR_WORD_BIT
		#define WAIT_FOR_WORD_BIT 1
	#endif
	#ifndef WAIT_FOR_NOT_WORD_BIT
		#define WAIT_FOR_NOT_WORD_BIT 1
	#endif
	#ifndef WAIT_FOR_DWORD_BIT
		#define WAIT_FOR_DWORD_BIT 1
	#endif
	#ifndef WAIT_FOR_NOT_DWORD_BIT
		#define WAIT_FOR_NOT_DWORD_BIT 1
	#endif
	#ifndef WAIT_FOR_SEM
		#define WAIT_FOR_SEM 1
	#endif
	#ifndef WAIT_FOR_COUNTING_SEM
		#define WAIT_FOR_COUNTING_SEM 1
	#endif
	#ifndef WAIT_FOR_ALMENO_UN_BIT
		#define WAIT_FOR_ALMENO_UN_BIT 1
	#endif
	#ifndef WAIT_FOR_NESSUN_BIT
		#define WAIT_FOR_NESSUN_BIT 1
	#endif
	#ifndef WAIT_FOR_ALMENO_UN_WORD_BIT
		#define WAIT_FOR_ALMENO_UN_WORD_BIT 1
	#endif
	#ifndef WAIT_FOR_NESSUN_WORD_BIT
		#define WAIT_FOR_NESSUN_WORD_BIT 1
	#endif
	#ifndef WAIT_FOR_ALMENO_UN_DWORD_BIT
		#define WAIT_FOR_ALMENO_UN_DWORD_BIT 1
	#endif
	#ifndef WAIT_FOR_NESSUN_DWORD_BIT
		#define WAIT_FOR_NESSUN_DWORD_BIT 1
	#endif

	#ifndef EXECUTION_CTR
		#define EXECUTION_CTR 1
	#endif
	#ifndef TASK_LABEL
		#define TASK_LABEL 64
	#endif
	#ifndef LOCAL_LAST_ERROR
		#define LOCAL_LAST_ERROR 1
	#endif
	#ifndef IDLE_TIME
		#define IDLE_TIME 1
	#endif
	#ifndef EVALUATE_FREE_STACK
		#define EVALUATE_FREE_STACK 1
	#endif
	#ifndef TIC_OBJs
		#define TIC_OBJs 1
	#endif
	#ifndef CONSTANT_SCHEDULING_TIME
		#define CONSTANT_SCHEDULING_TIME 1
	#endif
	#ifndef CALLER_ADDRESS
		#ifdef DEBUG
			#define CALLER_ADDRESS 1
		#else
			#define CALLER_ADDRESS 0
		#endif
	#endif

	#ifndef SCHEDULE_DIAG
		#define SCHEDULE_DIAG 0
	#endif

	#ifndef OUT_SYSTIC
		#define OUT_SYSTIC(VALUE)
	#endif

	#ifndef OUT_PENDVS
		#define OUT_PENDVS(VALUE)
	#endif

	#ifndef STACK_GUARD
		#define STACK_GUARD 1
	#endif

	#ifndef STACK_GUARD_PATTERN
		#define STACK_GUARD_PATTERN 0xA55A55AA
	#endif

	//	Timer protection: if scheduler priority differs from tic priority, RTK_SchedulerLock protection is surly insufficent
	#ifndef TIMER_INTERRUPT_PROTECT
		#define TIMER_INTERRUPT_PROTECT 0
	#endif

	#ifndef TIMER_NUMBER_CHECK
		#define TIMER_NUMBER_CHECK 1
	#endif

	#ifndef TIMERS_GUARD
		#define TIMERS_GUARD TIMER_NUMBER_CHECK
	#endif

#endif
