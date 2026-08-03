/*						ARK Project - Adaptive Runtime Kernel

	Module:
		SchedAsm.s

	Purpose:
		Low-level PendSV context-switch handler for the RTK scheduler.

	Description:
		This module implements the ARM Cortex-M PendSV handler used by RTK to switch task contexts.
		The handler saves the manually preserved part of the outgoing task context, invokes the
		scheduler selection path, restores the selected task context and returns from the exception.
		When stack guard checking is enabled, it also verifies the outgoing task stack guard before
		saving the context.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK versions.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include "../inc/RTK_Config.h"
#include "../inc/ErrCode.h"
#include "../inc/SchedAsmOffsets.h"

	.syntax unified         // use unified assembler syntax
	.code 16                // assemble in Thumb-2 (.thumb" can also be used)
	.arch armv7-m
	.fpu vfpv4

	.extern	CurrentTaskPtr	// Puntatore al descriptor della task in tiro
	.extern	RTK_UnrecoverableErrorEntry
	.global PendSV_Handler  // external linkage for my ISR name
 	.text                   // put into linker code section
	.thumb_func             // we are a thumb function
	.type PendSV_Handler, %function       // optional: mark it as a function
		


/*					PendSV_Handler

		Purpose:
			Handles the PendSV context-switch interrupt. This ISR saves the running task context, calls FirstToRun()
			to select the next task, and restores the selected task context.

		Author:
			Paolo Rozzi

		Reviewer:
			---

		Context:
			This ISR is triggered indirectly by RTK scheduling services while RTK is active.

		Input:
			CurrentTaskPtr: global pointer to the descriptor of the task being switched out.
			FirstToRun: scheduler routine called to select the next task descriptor.

		Output:
			Restores the selected task context and returns from the exception through EXC_RETURN in LR.

		Notes:
			- At this point, registers R0, R1, R2, R3, R12, LR, PC, and xPSR have already been automatically saved on
			  the process stack (PSP) by the hardware upon exception entry.
			- The stack in use during the ISR is the Main Stack Pointer (MSP).
			- The current value of LR does not represent a return address, but contains a special EXC_RETURN code that
			  encodes the processor state at the time the exception was taken.
			- The handler stores into the current task descriptor the registers that are not automatically saved by the
			  hardware. Among these, PSP is first read into R3 and then saved along with the other registers.
			- Bit 4 of EXC_RETURN indicates whether a floating-point context is present. If this bit is 0, S16-S31 must
			  be saved/restored by software. If it is 1, no floating-point context is involved.
			- In a future review, evaluate whether saving could be changed as follows:

				MRS		R1,PSP
				TST		LR,#0x10
				IT		EQ
				VSTMDBEQ	R1!,{S16-S31}
				STM		R1!,{R4-R11, LR}
				LDR		R0,= CurrentTaskPtr
				LDR		R0,[R0]
				STR		R1,[R0]

			  and whether restore could be changed as follows:

				LDR		R1, [R0]
				LDMIA		R1!, {R4-R11, LR}
				TST		LR, #0x10
				IT		EQ
				VLDMIAEQ	R1!, {S16-S31}
				MSR		PSP, R1

			  This change would also require reviewing the routines that create task descriptors.
*/
PendSV_Handler:	
		OUT_PENDVS(1)		// Macro to show this handler activity on a digital output

		LDR		R0,= CurrentTaskPtr
		LDR		R1,[R0]		// Now R1 contain the pointer to the descriptor of the current task

	#if STACK_GUARD
		LDR		R2,[R1,#TASK_STACK_GUARD_OFFSET]
		LDR		R12,=STACK_GUARD_PATTERN
		CMP		R2,R12
		BEQ		StackGuardOk
		LDR		R0,=RTK_FATAL_STACK_GUARD_ERROR
		B		RTK_UnrecoverableErrorEntry
StackGuardOk:
	#endif

		MRS		R3,PSP		// R3 has already been saved by hardware on ISR entry, so it is used as a pointer to the
 		ISB					// manual floating-point register save area. This R3 value is saved together with the other
							// manually saved registers, so it can be restored later.
		STM		R1!,{R3-R11, LR}
		TST		LR,#0x10
		IT		EQ
		VSTMDBEQ	R3!,{S16-S31}

		BL		FirstToRun		// Scelto il prossimo da runnare:

		// Arriviamo a questo punto con R0 che punta il task descriptor da eseguire. Si recuperano dal task descriptor i registri
		// che non sono salvati in automatico dall'interrupt fra i quali, in particolare, PSP (process stack pointer) che viene
 		// letto su R3, e quindi si esce dall'interrupt.
		LDM		R0!,{R3-R11, LR}
		MSR		PSP,R3
		ISB
		TST		LR,#0x10
		IT		EQ
		VLDMDBEQ	R3!,{S16-S31}

		OUT_PENDVS(0)		// Macro to show this handler activity on a digital output

		BX		LR			// RETI  (LR=R14=return address)
		// On BX LR, LR must contain an EXC_RETURN special value. Its low bits select whether the return pops the floating-point
		// context, returns to thread mode, and uses PSP instead of MSP. The return address is therefore taken from the stack
		// selected by EXC_RETURN, which is not necessarily the current stack.

	.END
