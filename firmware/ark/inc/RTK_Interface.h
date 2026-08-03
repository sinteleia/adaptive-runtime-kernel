/*						ARK Project - Adaptive Runtime Kernel

	Module:
		RTK_Interface.h

	Purpose:
		Project and processor adaptation interface required by RTK.

	Description:
		This header declares the porting functions used to adapt RTK behavior to the target processor
		and to project-specific requirements. In particular, the current interface covers the interrupt
		priority and BASEPRI setup required by the scheduler and system tick paths.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK hardware interface headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __RTK_INTERFACE_H
	#define __RTK_INTERFACE_H

	#include <stdint.h>
	#include "Type.h"

	#ifdef __cplusplus
		extern "C" {
	#endif

	/*									RTK_Interface
			This file declares the project-specific hardware interface used by RTK. The functions configure or restore the
		interrupt priorities used by the kernel and provide the BASEPRI thresholds required by RTK critical sections.
			SysTick and PendSV normally use different priorities so RTK can mask PendSV without stopping the system tick.
		They should usually be assigned the two lowest interrupt priorities available on the target core. PendSV must not
		be preempted by other ISRs. SysTick may be preempted by very heavy ISRs when system tick determinism requires it,
		but its priority should normally remain low because its execution time is not strictly constant. The system,
		including tick objects, must be dimensioned so SysTick handling remains shorter than the system tick period.
	*/

	// Return the BASEPRI value used by RTK to mask the scheduler interrupt during critical sections that cannot schedule.
	uint32_t RTK_GetSchedulerBasepri(void);

	// Return the BASEPRI value used by RTK to mask the system tick interrupt during critical sections that cannot accept it.
	uint32_t RTK_GetSysTicBasepri(void);

	// Configure PendSV with the lowest priority used by RTK.
	void SetPriorityPENDVS(void);

	// Configure SysTick with a priority above PendSV so the tick stays active when RTK masks PendSV through BASEPRI.
	void SetPrioritySysTic(void);

	// Start the periodic system tick used by RTK.
	void AttivaIlTic(void);

	// Stop the periodic system tick used by RTK.
	void DisattivaIlTic(void);

	// Restore the default SysTick priority.
	void ResetPrioritySysTic(void);

	// Restore the default PendSV priority.
	void ResetPriorityPENDVS(void);

	#ifdef __cplusplus
		}
	#endif

#endif
