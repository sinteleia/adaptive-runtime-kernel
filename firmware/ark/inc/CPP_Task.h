/*						ARK Project - Adaptive Runtime Kernel

	Module:
		CPP_Task.h

	Purpose:
		Public C++ base class for RTK tasks implemented as objects.

	Description:
		This header declares the T_CPP_Task base class used by C++ applications to bind an object
		lifetime and a virtual Task() method to an underlying RTK task descriptor.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK C++ task wrapper headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
/*
	T_CPP_Task

	Purpose:
		Base class for RTK tasks implemented as C++ objects.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		A derived class shall implement Task() and start execution by calling Run().
	Notes:
		The RTK task entry point is CPP_TaskExec(). It calls the derived Task() method, clears Handle, and deletes the object when
		Task() returns. This supports the preferred termination model, where the task exits by itself and the object is destroyed by
		the task wrapper.

		If an object is destroyed while the associated RTK task is still active, the base destructor kills the RTK task through Handle.
		For an orderly externally requested shutdown, the derived class should use an application-owned termination flag, let Task()
		release its resources and return, and let CPP_TaskExec() complete object destruction.
*/

#ifndef CPP_TASK_H_
	#define CPP_TASK_H_

	#ifdef __cplusplus

		#include "Sched.h"
		class T_CPP_Task{
			public:
				T_CPP_Task(void);
				virtual ~T_CPP_Task();
				bool Run(const char *Nome, short StackSize, T_TaskPriority TaskPriority);
				virtual void Task(void)=0;
				void SetTaskName(const char *Nome);
				T_TaskDescriptor *Handle;
		};

	#endif

#endif
