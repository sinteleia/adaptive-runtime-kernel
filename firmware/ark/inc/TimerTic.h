/*						ARK Project - Adaptive Runtime Kernel

	Module:
		TimerTic.h

	Purpose:
		Public and internal interface for RTK timer queue processing.

	Description:
		This header declares the timer queue state and services used to initialize, arm, disarm,
		process and diagnose RTK timers driven by the system tick.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK timer tick headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __TIMER_TIC_h
	#define __TIMER_TIC_h

	#include "RTK_Config.h"

	#include "Type.h"
	#include "Timer.h"

	extern T_Timer *FirstToTic;
	extern volatile DWORD TimerCtr;

	#ifdef __cplusplus
		extern "C" {
	#endif
 
	void InitTimerTic(void);
	DWORD TimerTicQuantoManca(T_Timer *T);
	void TimerTic(void);
	void SetTimer(T_Timer *TimerToSet, DWORD TicToWait);
	void DisarmaTimer(T_Timer *TimerToDelete);
	T_TimerStatus CheckTimerStatus();

	#ifdef __cplusplus
		}
	#endif
 
#endif
