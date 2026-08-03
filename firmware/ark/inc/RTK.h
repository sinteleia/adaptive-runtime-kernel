/*						ARK Project - Adaptive Runtime Kernel

	Module:
		RTK.h

	Purpose:
		Main aggregate include for RTK application-facing services.

	Description:
		This header collects the primary RTK and ARK service headers used by applications and tests,
		including scheduler, waits, timers, diagnostics, memory management, semaphores and queue
		support. It does not define behavior directly; it provides a single include point for the
		kernel interface set.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK aggregate include headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef RTK_H_
	#define RTK_H_

	#include "ErrCode.h"
	#include "LastErr.h"
	#include "MM.H"
	#include "RTK_Wait.h"
	#include "Sched.h"
	#include "Sem.h"
	#include "TaskDiag.h"
	#include "Tic.h"
	#include "TimeOut.h"
	#include "timer.h"
	#include "TimerTic.h"
	#include "Pack16.h"
	#include "RTK_Interface.h"
	#include "QueDWord.h"
	#include "QueWord.h"
	#include "QueByte.h"

#endif
