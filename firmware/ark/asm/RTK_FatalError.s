/*						ARK Project - Adaptive Runtime Kernel

	Module:
		RTK_FatalError.s

	Purpose:
		Low-level unrecoverable error entry for RTK fatal paths.

	Description:
		This module provides the assembly entry point used when RTK detects an unrecoverable error
		or a CPU fault handler must enter the RTK fatal path. The entry masks interrupts and faults,
		restores the startup stack pointer, forces stack use back to MSP and branches to the C fatal
		error dispatch routine without relying on the current task stack.

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
/*
	RTK_UnrecoverableErrorEntry

	Purpose:
		Enter the RTK unrecoverable error path without relying on the current task stack.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by RTK assembly code or CPU fault handlers after detecting a non-recoverable runtime error.
	Input:
		R0 - Unrecoverable error reason code, preserved and passed to RTK_UnrecoverableErrorDispatch().
	Output:
		Does not return.
	Notes:
		This entry masks interrupts and configurable faults, restores the startup MSP value from _estack, forces thread mode
		to use MSP if execution ever reaches thread context again, and branches to the C dispatch routine.
*/

	.syntax unified
	.code 16
	.arch armv7-m
	.fpu vfpv4

	.extern _estack
	.extern RTK_UnrecoverableErrorDispatch

	.global RTK_UnrecoverableErrorEntry
	.text
	.thumb_func
	.type RTK_UnrecoverableErrorEntry, %function

RTK_UnrecoverableErrorEntry:
	CPSID	i
	CPSID	f
	LDR		R1,=_estack
	MSR		MSP,R1
	MSR		PSP,R1
	MRS		R2,CONTROL
	BIC		R2,R2,#2
	MSR		CONTROL,R2
	ISB
	MOV		SP,R1
	B		RTK_UnrecoverableErrorDispatch

	.END
