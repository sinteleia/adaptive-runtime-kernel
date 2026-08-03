#include "main.h"
#include "RTK_Interface.h"

#define RTK_PENDSV_PRIO_LOGICAL ((1UL << __NVIC_PRIO_BITS) - 1UL)
#define RTK_SYSTICK_PRIO_LOGICAL (RTK_PENDSV_PRIO_LOGICAL - 1UL)
#define RTK_PENDSV_BASEPRI (RTK_PENDSV_PRIO_LOGICAL << (8U - __NVIC_PRIO_BITS))
#define RTK_SYSTICK_BASEPRI (RTK_SYSTICK_PRIO_LOGICAL << (8U - __NVIC_PRIO_BITS))

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
	(void)TickPriority;
	return HAL_OK;
}

uint32_t RTK_GetSchedulerBasepri(void) {
	return RTK_PENDSV_BASEPRI;
}



/*
					RTK_GetSysTicBasepri

	Purpose:
		Return the BASEPRI value used to mask SysTick and lower-priority RTK interrupts.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by RTK code that must protect data shared with the system tick interrupt.
	Input:
		None.
	Output:
		BASEPRI threshold corresponding to the configured SysTick interrupt priority.
	Notes:
		On Cortex-M, BASEPRI masks interrupts with numerical priority greater than or equal to the threshold.
*/
uint32_t RTK_GetSysTicBasepri(void) {
	return RTK_SYSTICK_BASEPRI;
}

void SetPriorityPENDVS(void) {
	NVIC_SetPriority(PendSV_IRQn, RTK_PENDSV_PRIO_LOGICAL);
}

void SetPrioritySysTic(void) {
	NVIC_SetPriority(SysTick_IRQn, RTK_SYSTICK_PRIO_LOGICAL);
}

void AttivaIlTic(void) {
	(void)SysTick_Config(SystemCoreClock / 1000U);
}

void DisattivaIlTic(void) {
	SysTick->CTRL=SysTick_CTRL_CLKSOURCE_Msk;
}

void ResetPrioritySysTic(void) {
	NVIC_SetPriority(SysTick_IRQn, 0U);
}

void ResetPriorityPENDVS(void) {
	NVIC_SetPriority(PendSV_IRQn, 0U);
}
