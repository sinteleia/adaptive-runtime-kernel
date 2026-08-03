#include "MicroDelay.h"

extern DWORD SystemCoreClock;

static DWORD uS_Const;

/*
					Init_uS_ToDelay

	Purpose:
		Enable the Cortex-M DWT cycle counter and compute the cycles-per-microsecond conversion factor.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called once before using MicroDelay() on Cortex-M cores that implement DWT CYCCNT.
	Input:
		None.
	Output:
		true when the cycle counter is available and enabled; false otherwise.
	Notes:
		DWT CYCCNT is present on Cortex-M3 and later mainline cores, but may be absent or disabled on some profiles.
*/
bool Init_uS_ToDelay(void) {
	CORE_DEMCR|=CORE_DEMCR_TRCENA;
	if((DWT_CTRL&DWT_CTRL_NOCYCCNT)!=0U) return false;
	DWT_CYCCNT=0U;
	DWT_CTRL|=DWT_CTRL_CYCCNTENA;
	uS_Const=SystemCoreClock / 1000000;
	return uS_Const!=0U;
}

/*
					MicroDelay

	Purpose:
		Busy-wait for the requested number of microseconds using the DWT cycle counter.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called after Init_uS_ToDelay() has enabled and calibrated DWT CYCCNT.
	Input:
		uS_ToDelay - Delay duration in microseconds.
	Output:
		The routine returns after the requested cycle interval has elapsed.
*/
void MicroDelay(DWORD uS_ToDelay) {
	DWORD Old=DWT_CYCCNT;
	DWORD n=uS_ToDelay*uS_Const;

	while((DWORD)(DWT_CYCCNT-Old)<n);
}
