//	VERSION 0.8
        .syntax unified         // use unified assembler syntax 
	.code 16                // assemble in Thumb-2 (.thumb" can also be used)
        .arch armv7-m
        .fpu vfpv4

	.extern	HardFault_GetContext
        .global HardFault_Handler       // external linkage for my ISR name 
        .text                           // put into linker code section
        .thumb_func                     // we are a thumb function 
        .type   HardFault_Handler, %function       // optional: mark it as a function 
/*
	MODULE	ErrorTrapAsm

	EXTERN	HardFault_GetContext
	PUBLIC	HardFault_Handler
	
	SECTION	.text:CODE:ROOT
*/
/*    HardFault_Handler
    Determines whether the faulted context used PSP or MSP from EXC_RETURN bit 2.
  Calls HardFault_GetContext(), which does not return, passing the selected
  stacked context pointer, EXC_RETURN, PSP and MSP.
*/
HardFault_Handler:
	MOV	R1,LR
	MRS	R2,PSP
	MRS	R3,MSP
	MOVS	R0,#4
	TST	R0,R1
	BEQ	IS_MSP
		MOV	R0,R2
		BL	HardFault_GetContext
		BX	LR      // RETI  (LR=R14=return address)
	IS_MSP:
		MOV	R0,R3
		BL	HardFault_GetContext
		BX	LR      // RETI  (LR=R14=return address)

	.END
