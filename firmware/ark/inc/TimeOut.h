/*						ARK Project - Adaptive Runtime Kernel

	Module:
		TimeOut.h

	Purpose:
		Lightweight timeout helpers based on the RTK system tick counter.

	Description:
		This header defines timeout values and inline helpers for deterministic timeout checks based on
		TimerCtr. Timeout values are arithmetic expiration timestamps, not timer objects, and are not
		inserted in the RTK timer queue.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK timeout helpers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __TimeOut_h
	#define __TimeOut_h

	#include "Type.h"
	#include "TimerTic.h"

	/*				Timeout facility

		Purpose:
			Provide lightweight timeout values expressed in system ticks.
		Author:
			Paolo Rozzi
		Reviewer:
			---
		Context:
			Available only while RTK is running, or when a system tick handler increments TimerCtr every millisecond.

		Notes:
				Timeout values are not timers. They use less memory and, because they are not stored in a list, setting or
			updating them has deterministic execution time. Compared with timers, they are affected by TimerCtr wraparound;
			users must check them at intervals shorter than half of the maximum TimerCtr range.
				The maximum representable timeout depends on the size of TimerCtr.

	*/

	// TimerCtr is volatile, but timeout values are local arithmetic copies; +0U keeps the numeric type and drops volatile.
	typedef __typeof__(TimerCtr+0U) T_TO;

	static inline bool IsToElapsed(T_TO TO) {
		return ((int)(TO-TimerCtr)<0);
	}

	static inline bool IsNotElapsedTO(T_TO TO) {
		return ((int)(TO-TimerCtr)>0);
	}

	static inline void PresetTO(T_TO *TO, T_TO Tic) {
		*TO=Tic+TimerCtr;
	}

	static inline void AddToTO(T_TO *TO, T_TO Tic) {
		(*TO)+=Tic;
	}

	#ifdef __cplusplus

		static inline T_TO PresetTO(T_TO Tic) {
			return Tic+TimerCtr;
		}

		static inline void PresetTO(T_TO &TO, T_TO Tic) {
			TO=Tic+TimerCtr;
		}

	#endif

#endif
