/*						Mylib Support Library

	Module:
		MyIntrinsics.h

	Purpose:
		ARM/CMSIS intrinsic wrappers and low-level protection helpers.

	Description:
		This header includes the compiler intrinsic definitions used by Mylib and projects built on
		top of it. It provides interrupt masking macros, link-register access and inline exclusive-access
		helpers for atomic byte, word and dword updates on ARM Cortex-M targets.

		On ARM Cortex-M, these atomic helpers are based on exclusive access sequences: LDREX reads the
		current value and marks the addressed location for exclusive access, while STREX attempts the
		write and reports whether the exclusive monitor was invalidated. The operation is retried when
		the exclusive write fails. Atomicity therefore comes from the exclusive monitor protocol, not
		from a single monolithic read-modify-write instruction.

	Mylib version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older Mylib intrinsic helpers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __MyIntrinsics_h
 #define __MyIntrinsics_h
 
 #include <stdint.h>

 #if defined(__CC_ARM)
  #include "cmsis_armcc.h"
 #elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) && (__ARMCC_VERSION < 6100100)
  #include "cmsis_armclang_ltm.h"
 #elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6100100)
  #include "cmsis_armclang.h"
 #elif defined(__GNUC__)
  #include "cmsis_gcc.h"
 #elif defined(__ICCARM__)
  #include "cmsis_iccarm.h"
 #else
  #include "cmsis_compiler.h"
 #endif

 #include "Type.h"
 
 #define START_PROTECTION	int s = __get_PRIMASK(); \
							__disable_irq();\
							__DMB();

							
 #define RESTART_PROTECTION	s = __get_PRIMASK(); \
							__disable_irq();\
							__DMB();

							
 #define END_PROTECTION		__DMB();\
							__set_PRIMASK(s);

	__attribute__( ( always_inline ) ) inline uint32_t __get_LR(void){
		register uint32_t result;
		__ASM volatile ("MOV %0, LR\n" : "=r" (result) );
		return(result);
	}

	/*
					RTK_ExclusiveIncrementByte

		Purpose:
			Atomically increment an 8-bit volatile value using ARM exclusive access instructions.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Called when a shared byte counter must be updated without taking a scheduler lock.
		Input:
			Value - Pointer to the volatile byte value to increment.
		Output:
			The value stored after the successful exclusive update.
	*/
	__attribute__((always_inline)) static inline BYTE RTK_ExclusiveIncrementByte(volatile BYTE *Value) {
		BYTE OldValue;
		BYTE NewValue;

		do {
			OldValue=(BYTE)__LDREXB((volatile uint8_t *)Value);
			NewValue=(BYTE)(OldValue+1U);
		} while(__STREXB((uint8_t)NewValue, (volatile uint8_t *)Value)!=0U);
		return NewValue;
	}

	/*
					RTK_ExclusiveDecrementByte

		Purpose:
			Atomically decrement an 8-bit volatile value using ARM exclusive access instructions.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Called when a shared byte counter must be updated without taking a scheduler lock.
		Input:
			Value - Pointer to the volatile byte value to decrement.
		Output:
			The value stored after the successful exclusive update.
	*/
	__attribute__((always_inline)) static inline BYTE RTK_ExclusiveDecrementByte(volatile BYTE *Value) {
		BYTE OldValue;
		BYTE NewValue;

		do {
			OldValue=(BYTE)__LDREXB((volatile uint8_t *)Value);
			NewValue=(BYTE)(OldValue-1U);
		} while(__STREXB((uint8_t)NewValue, (volatile uint8_t *)Value)!=0U);
		return NewValue;
	}

	/*
					RTK_ExclusiveDecrementByteIfNotZero

		Purpose:
			Atomically decrement an 8-bit volatile value only when it is not zero.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Called when a shared byte counter must not underflow.
		Input:
			Value - Pointer to the volatile byte value to decrement.
		Output:
			The current value, or zero when the value was already zero.
	*/
	__attribute__((always_inline)) static inline BYTE RTK_ExclusiveDecrementByteIfNotZero(volatile BYTE *Value) {
		BYTE OldValue;
		BYTE NewValue;

		do {
			OldValue=(BYTE)__LDREXB((volatile uint8_t *)Value);
			if(OldValue==0U) {
				__CLREX();
				return 0U;
			}
			NewValue=(BYTE)(OldValue-1U);
		} while(__STREXB((uint8_t)NewValue, (volatile uint8_t *)Value)!=0U);
		return NewValue;
	}

	/*
					RTK_ExclusiveIncrementWord

		Purpose:
			Atomically increment a 16-bit volatile value using ARM exclusive access instructions.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Called when a shared word counter must be updated without taking a scheduler lock.
		Input:
			Value - Pointer to the volatile word value to increment.
		Output:
			The value stored after the successful exclusive update.
	*/
	__attribute__((always_inline)) static inline WORD RTK_ExclusiveIncrementWord(volatile WORD *Value) {
		WORD OldValue;
		WORD NewValue;

		do {
			OldValue=(WORD)__LDREXH((volatile uint16_t *)Value);
			NewValue=(WORD)(OldValue+1U);
		} while(__STREXH((uint16_t)NewValue, (volatile uint16_t *)Value)!=0U);
		return NewValue;
	}

	/*
					RTK_ExclusiveDecrementWord

		Purpose:
			Atomically decrement a 16-bit volatile value using ARM exclusive access instructions.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Called when a shared word counter must be updated without taking a scheduler lock.
		Input:
			Value - Pointer to the volatile word value to decrement.
		Output:
			The value stored after the successful exclusive update.
	*/
	__attribute__((always_inline)) static inline WORD RTK_ExclusiveDecrementWord(volatile WORD *Value) {
		WORD OldValue;
		WORD NewValue;

		do {
			OldValue=(WORD)__LDREXH((volatile uint16_t *)Value);
			NewValue=(WORD)(OldValue-1U);
		} while(__STREXH((uint16_t)NewValue, (volatile uint16_t *)Value)!=0U);
		return NewValue;
	}

	/*
					RTK_ExclusiveDecrementWordIfNotZero

		Purpose:
			Atomically decrement a 16-bit volatile value only when it is not zero.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Called when a shared word counter must not underflow.
		Input:
			Value - Pointer to the volatile word value to decrement.
		Output:
			The current value, or zero when the value was already zero.
	*/
	__attribute__((always_inline)) static inline WORD RTK_ExclusiveDecrementWordIfNotZero(volatile WORD *Value) {
		WORD OldValue;
		WORD NewValue;

		do {
			OldValue=(WORD)__LDREXH((volatile uint16_t *)Value);
			if(OldValue==0U) {
				__CLREX();
				return 0U;
			}
			NewValue=(WORD)(OldValue-1U);
		} while(__STREXH((uint16_t)NewValue, (volatile uint16_t *)Value)!=0U);
		return NewValue;
	}

	/*
					RTK_ExclusiveIncrementDword

		Purpose:
			Atomically increment a 32-bit volatile value using ARM exclusive access instructions.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Called when a shared double-word counter must be updated without taking a scheduler lock.
		Input:
			Value - Pointer to the volatile double-word value to increment.
		Output:
			The value stored after the successful exclusive update.
	*/
	__attribute__((always_inline)) static inline DWORD RTK_ExclusiveIncrementDword(volatile DWORD *Value) {
		DWORD OldValue;
		DWORD NewValue;

		do {
			OldValue=(DWORD)__LDREXW((volatile uint32_t *)Value);
			NewValue=(DWORD)(OldValue+1UL);
		} while(__STREXW((uint32_t)NewValue, (volatile uint32_t *)Value)!=0U);
		return NewValue;
	}

	/*
					RTK_ExclusiveDecrementDword

		Purpose:
			Atomically decrement a 32-bit volatile value using ARM exclusive access instructions.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Called when a shared double-word counter must be updated without taking a scheduler lock.
		Input:
			Value - Pointer to the volatile double-word value to decrement.
		Output:
			The value stored after the successful exclusive update.
	*/
	__attribute__((always_inline)) static inline DWORD RTK_ExclusiveDecrementDword(volatile DWORD *Value) {
		DWORD OldValue;
		DWORD NewValue;

		do {
			OldValue=(DWORD)__LDREXW((volatile uint32_t *)Value);
			NewValue=(DWORD)(OldValue-1UL);
		} while(__STREXW((uint32_t)NewValue, (volatile uint32_t *)Value)!=0U);
		return NewValue;
	}

	/*
					RTK_ExclusiveDecrementDwordIfNotZero

		Purpose:
			Atomically decrement a 32-bit volatile value only when it is not zero.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Called when a shared double-word counter must not underflow.
		Input:
			Value - Pointer to the volatile double-word value to decrement.
		Output:
			The current value, or zero when the value was already zero.
	*/
	__attribute__((always_inline)) static inline DWORD RTK_ExclusiveDecrementDwordIfNotZero(volatile DWORD *Value) {
		DWORD OldValue;
		DWORD NewValue;

		do {
			OldValue=(DWORD)__LDREXW((volatile uint32_t *)Value);
			if(OldValue==0UL) {
				__CLREX();
				return 0UL;
			}
			NewValue=(DWORD)(OldValue-1UL);
		} while(__STREXW((uint32_t)NewValue, (volatile uint32_t *)Value)!=0U);
		return NewValue;
	}

	#ifdef __cplusplus
		static inline BYTE RTK_ExclusiveIncrement(volatile BYTE &Value) {
			return RTK_ExclusiveIncrementByte(&Value);
		}

		static inline WORD RTK_ExclusiveIncrement(volatile WORD &Value) {
			return RTK_ExclusiveIncrementWord(&Value);
		}

		static inline DWORD RTK_ExclusiveIncrement(volatile DWORD &Value) {
			return RTK_ExclusiveIncrementDword(&Value);
		}

		static inline BYTE RTK_ExclusiveDecrement(volatile BYTE &Value) {
			return RTK_ExclusiveDecrementByte(&Value);
		}

		static inline WORD RTK_ExclusiveDecrement(volatile WORD &Value) {
			return RTK_ExclusiveDecrementWord(&Value);
		}

		static inline DWORD RTK_ExclusiveDecrement(volatile DWORD &Value) {
			return RTK_ExclusiveDecrementDword(&Value);
		}

		static inline BYTE RTK_ExclusiveDecrementIfNotZero(volatile BYTE &Value) {
			return RTK_ExclusiveDecrementByteIfNotZero(&Value);
		}

		static inline WORD RTK_ExclusiveDecrementIfNotZero(volatile WORD &Value) {
			return RTK_ExclusiveDecrementWordIfNotZero(&Value);
		}

		static inline DWORD RTK_ExclusiveDecrementIfNotZero(volatile DWORD &Value) {
			return RTK_ExclusiveDecrementDwordIfNotZero(&Value);
		}
	#endif
#endif
