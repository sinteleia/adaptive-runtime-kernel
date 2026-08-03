/*						ARK Project - Adaptive Runtime Kernel

	Module:
		SchedAsmOffsets.h

	Purpose:
		Assembly-visible task descriptor offsets used by RTK scheduler assembly code.

	Description:
		Assembly sources cannot directly use the offsets of fields in a structure defined in C.
		The practical way for an assembly routine to access those fields is to redefine the required
		offsets here, then make one C/C++ source-visible check fail at compile time if the
		compiler-generated offsets and the offsets defined in this file differ.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Created for ARK 1.0 to keep scheduler assembly offsets explicit and checked.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __SCHED_ASM_OFFSETS_H
	#define __SCHED_ASM_OFFSETS_H

	#define SCHED_ASM_ALIGN_UP(V, A) (((V)+((A)-1))&(~((A)-1)))

	#define TASK_STACK_GUARD_BASE_OFFSET 126

	#if EXECUTION_CTR
		#define TASK_STACK_GUARD_AFTER_TASK_CTR_OFFSET 128
	#else
		#define TASK_STACK_GUARD_AFTER_TASK_CTR_OFFSET TASK_STACK_GUARD_BASE_OFFSET
	#endif

	#if TASK_LABEL == 0
		#define TASK_STACK_GUARD_AFTER_LABEL_OFFSET TASK_STACK_GUARD_AFTER_TASK_CTR_OFFSET
	#elif TASK_LABEL == 16
		#define TASK_STACK_GUARD_AFTER_LABEL_OFFSET (SCHED_ASM_ALIGN_UP(TASK_STACK_GUARD_AFTER_TASK_CTR_OFFSET, 2)+2)
	#elif TASK_LABEL == 32
		#define TASK_STACK_GUARD_AFTER_LABEL_OFFSET (SCHED_ASM_ALIGN_UP(TASK_STACK_GUARD_AFTER_TASK_CTR_OFFSET, 4)+4)
	#elif TASK_LABEL == 64
		#define TASK_STACK_GUARD_AFTER_LABEL_OFFSET (SCHED_ASM_ALIGN_UP(TASK_STACK_GUARD_AFTER_TASK_CTR_OFFSET, 8)+8)
	#endif

	#if LOCAL_LAST_ERROR
		#define TASK_STACK_GUARD_AFTER_LAST_ERROR_OFFSET (SCHED_ASM_ALIGN_UP(TASK_STACK_GUARD_AFTER_LABEL_OFFSET, 2)+2)
	#else
		#define TASK_STACK_GUARD_AFTER_LAST_ERROR_OFFSET TASK_STACK_GUARD_AFTER_LABEL_OFFSET
	#endif

	#if IDLE_TIME
		#define TASK_STACK_GUARD_AFTER_IDLE_TIME_OFFSET (SCHED_ASM_ALIGN_UP(TASK_STACK_GUARD_AFTER_LAST_ERROR_OFFSET, 4)+4)
	#else
		#define TASK_STACK_GUARD_AFTER_IDLE_TIME_OFFSET TASK_STACK_GUARD_AFTER_LAST_ERROR_OFFSET
	#endif

	#if CALLER_ADDRESS
		#define TASK_STACK_GUARD_AFTER_CALLER_OFFSET (SCHED_ASM_ALIGN_UP(TASK_STACK_GUARD_AFTER_IDLE_TIME_OFFSET, 4)+4)
	#else
		#define TASK_STACK_GUARD_AFTER_CALLER_OFFSET TASK_STACK_GUARD_AFTER_IDLE_TIME_OFFSET
	#endif

	#define TASK_STACK_GUARD_OFFSET SCHED_ASM_ALIGN_UP(TASK_STACK_GUARD_AFTER_CALLER_OFFSET, 4)

#endif
