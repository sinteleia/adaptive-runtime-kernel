/*						ARK Project - Adaptive Runtime Kernel

	Module:
		CPP_Task.cpp

	Purpose:
		C++ object wrapper implementation for RTK tasks.

	Description:
		This module implements the C++ task wrapper used to run derived C++ objects as RTK tasks.
		It creates the underlying RTK task descriptor, invokes the derived Task() method through a C
		entry point and coordinates task/object destruction when the task returns or the object is
		destroyed while still active.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK C++ task wrapper implementations.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include "CPP_Task.h"
#include "RTK.h"
#include "stdio.h"

/*
	CPP_TaskExec

	Purpose:
		Execute the Task() method of a C++ task object and destroy the object when Task() returns.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by the RTK scheduler as the entry point of a task created by T_CPP_Task::Run().
	Input:
		CPP_Task - Pointer to the C++ task object to execute.
	Output:
		The object Handle is cleared and the C++ task object is deleted after Task() returns.
	Notes:
		Clearing Handle prevents the T_CPP_Task destructor from killing an already returned RTK task.
*/
extern "C" void CPP_TaskExec(T_CPP_Task *CPP_Task){
	CPP_Task->Task();
	CPP_Task->Handle=NULL;
	delete CPP_Task;
}

#define TRACEINFO(...){}

/*
	T_CPP_Task::T_CPP_Task

	Purpose:
		Initialize the C++ task wrapper before it is associated with an RTK task descriptor.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called when a derived C++ task object is constructed.
	Input:
		None.
	Output:
		Handle is initialized to NULL.
*/
T_CPP_Task::T_CPP_Task(){
	Handle=NULL;
}

/*
	T_CPP_Task::Run

	Purpose:
		Create and start the RTK task associated with this C++ task object.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by application code after constructing a derived T_CPP_Task object.
	Input:
		Nome - Logical task name to store in the RTK task descriptor.
		StackSize - Stack size requested for the RTK task.
		TaskPriority - Priority assigned to the RTK task.
	Output:
		true when the RTK task descriptor is created, false otherwise. Handle is updated with the returned descriptor pointer.
	Notes:
		The current object pointer is passed to CPP_TaskExec() as the RTK task parameter.
*/
bool T_CPP_Task::Run(const char *Nome, short StackSize, T_TaskPriority TaskPriority){
	TRACEINFO("create CPP Task object.\n");
	Handle=CreateNamedParTask((FuncPar)CPP_TaskExec, (DWORD)this, RTK_Pack(Nome), StackSize, TaskPriority);
	return Handle!=NULL;
}

/*
	T_CPP_Task::~T_CPP_Task

	Purpose:
		Destroy the C++ task wrapper and stop the associated RTK task when it is still active.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called when a T_CPP_Task object or a derived object is destroyed.
	Input:
		None.
	Output:
		If Handle is not NULL, the associated RTK task is killed while the scheduler is locked.
	Notes:
		When Task() returns normally through CPP_TaskExec(), Handle is cleared before deletion and KillTask() is not called here.
*/
T_CPP_Task::~T_CPP_Task(){
	uint32_t TicOn=RTK_SchedulerLock();
	TRACEINFO("destroy CPP Task object.\n");
	if(Handle!=NULL) KillTask(Handle);
	RTK_Unlock(TicOn);
}


/*
	T_CPP_Task::SetTaskName

	Purpose:
		Set or change the logical name stored in the associated RTK task descriptor.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called after Run() has created the RTK task descriptor.
	Input:
		Nome - Logical task name to store in the descriptor.
	Output:
		Handle->Label is updated.
	Notes:
		The caller shall ensure that Handle is valid before calling this routine.
*/
void T_CPP_Task::SetTaskName(const char *Nome){
	Handle->Label=RTK_Pack(Nome);
}
