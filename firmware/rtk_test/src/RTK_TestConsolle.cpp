#include "RTK_TestConsolle.h"
#include <cstdio>
#include "RTK_TestBoard.h"
#include "rtk.h"
#include "RTK_Noise.h"
#include "string.h"
#include "General.h"

#define NOISE_MEMORY_TASK_COUNT 10
#define NOISE_TIMER_TASK_COUNT 10
#define NOISE_FLOAT_TASK_COUNT 4

#define PRINT(...) printf(__VA_ARGS__);

#define CLRSCR "\x1B[2J"
#define HOME "\x1B[H"
#define MAX_TASK_FOR_LIST 30

T_TaskDescriptor *PrintStatusTaskHND;
static const char Cause[]={
	"None              \0"
	"Time              \0"
	"Ever              \0"
	"Semaphore         \0"
	"Counting Sem      \0"
	"Flag              \0"
	"not Flag          \0"
	"Que Get           \0"
	"Bynary len Que Put\0"
	"Free len Que Put  \0"
	"Que empty         \0"
	"BYTE bit          \0"
	"BYTE not bit      \0"
	"WORD bit          \0"
	"WORD not bit      \0"
	"DWORD bit         \0"
	"DWORD not bit     \0"
	"CAN Que Space     \0"
	"CAN Que Message   \0"
	"Invalid           \0"
};

static T_TaskDescriptor **T;
static T_TaskDiagStatus TaskDiagStatus;
extern unsigned int NumberOfActiveTimers; // This variable normally is not public
T_TaskTO_Check *TaskTO_Check[NOISE_TIMER_TASK_COUNT];

/*
					PrintCause

	Purpose:
		Print the text associated with an RTK wait cause code.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by PrintTaskList while printing the interactive console task status page.
	Input:
		c: RTK wait cause code. Bit 7 is ignored before indexing the table.
	Output:
		None.
	Notes:
		Out-of-range values are printed as Invalid.
*/
static inline void PrintCause(char c) {
	c&=0x7F;
	if(c>InvalidWait) {
		c=InvalidWait;
	}
	PRINT(&Cause[(strlen(Cause)+1)*c])
}

/*
					PrintTaskList

	Purpose:
		Print a diagnostic table for one RTK scheduler task list.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by PrintStatusTask when the operator requests the task status page.
	Input:
		L: scheduler list head to inspect.
		S: text label for the scheduler priority class.
	Output:
		None.
	Notes:
		The current task is skipped to keep the displayed task list focused on the monitored workload.
*/
static void PrintTaskList(T_TaskDescriptor *L, const char *S) {
	int N=GetDescriptorsPointers(T, MAX_TASK_FOR_LIST, L);
	if(N) {
		printf(" - %i %s priority task:\n\r", N, S);
		printf("   Task name     stopped @ for mS.    Waiting event      address  mS to wait Run ctr    free\n\r");
		while(N) {
			N--;
			if(T[N]!=CurrentTaskPtr) {
				GetTaskDiagStatus(T[N], &TaskDiagStatus);
				char Str[13];
				UnPack64(TaskDiagStatus.Label, Str);
				Str[12]=0;
				printf("  \x22%s\x22" , Str);
				printf(" %8p", (void *)(TaskDiagStatus.StopAddress));
				printf("  %-10lu ", TaskDiagStatus.IdleTime);
				PrintCause(TaskDiagStatus.WaitingType);
				if((TaskDiagStatus.WaitingType&0x7f)>WaitingForTime) {
					printf(" %8p ", TaskDiagStatus.AddressOfWaitingObject);
				} else {
					printf(" -------- ");
				}
				if((TaskDiagStatus.WaitingType&0x80) || (TaskDiagStatus.WaitingType==WaitingForTime)) {
					printf("%-10lu", TaskDiagStatus.IdleTime);
				} else {
					printf("forever   ");
				}
				printf(" %-10i", TaskDiagStatus.RunCtr);
				printf(" %i", TaskDiagStatus.MinUnusedStackDWords);
				printf(";\n\r");
			}
		}
	} else {
		printf(" - No %s priority task:\n\r", S);
	}
}

/*
					PrintStatusTask

	Purpose:
		Run the interactive console task used to inspect memory and scheduler status during the concurrence test.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Created by RTK_TestConsolle as a high-priority task.
	Input:
		Par: pointer to the termination flag owned by RTK_TestConsolle.
	Output:
		None.
	Notes:
		The task loops on console commands until the operator presses Q, then sets the termination flag.
*/
void PrintStatusTask(unsigned long Par) {
	bool *Terminato=(bool *)Par;
	static BYTE c='?';
	do {
		switch(c) {
			case '?': case 'H': case 'h':
				PRINT( HOME CLRSCR"                  Functionality test\n\r"
						          "  During this test many different task are running and working with malloc, timers and waits and the user can monitor memory occupation and scheduler activity.\n\r"
						          "    Debug functions:\n\r"
						          "     M for memory status;\n\r"
						          "     T for task status;\n\r"
				                  "     D for timer status;\n\r"
				                  "     R for random number;\n\r"
						          "     Q for terminate this test;\n\r"
				                  "     ? for this help;\n\r");
				break;
			case 'M': case 'm':
				{
					T_Len HeapDimension;
					T_Len MaxBlockDimension;
					int NumOfFreeBlock;
					int NumOfAllocatedBlock;
					PRINT(HOME CLRSCR "Memory status:\r\n - Heap status is: ")
					switch(HeapStatus(&HeapDimension, &MaxBlockDimension, &NumOfFreeBlock, &NumOfAllocatedBlock)) {
						case HeapOk: PRINT("HeapOk") break;
						case BlkAddressError: PRINT("block address error") break;
						case BlkOutOfHeapMemory: PRINT("BlkOutOfHeapMemory") break;
						case BlkSizeError: PRINT("BlkSizeError") break;
						case BlocchiLiberiAdiacenti: PRINT("BlocchiLiberiAdiacenti") break;
						case BlocchiLiberiSovrapposti: PRINT("BlocchiLiberiSovrapposti") break;
						case GuardiaSfondata: PRINT("Scrittura fuori dal payload") break;
						case BlockNumberError: PRINT("BlockNumberError") break;
					}
					PRINT(";\n\r - Total free memory: %lu bytes;\n\r", HeapDimension)
					PRINT(" - Max block dimension: %lu bytes;\n\r", MaxBlockDimension)
					PRINT(" - Num of free block: %u;\n\r", NumOfFreeBlock)
					PRINT(" - Num of detected allocated block: %u;\n\r", NumOfAllocatedBlock)
					#ifdef BLOCK_COUNTER
						PRINT(" - Num of theoretical allocated block: %u;\n\r", BlockCounter)
						PRINT(" - Total num of malloc executed: %lu;\n\r", TotalAllocExecuted)
					#endif
				}
				break;
			case 'T': case 't':
				PRINT(HOME CLRSCR "Task status:\r\n")
				T=(T_TaskDescriptor **)malloc(MAX_TASK_FOR_LIST*sizeof(T_TaskDescriptor *));
				if(T) {
					PrintTaskList((T_TaskDescriptor *)CriticalProcList, "critical");
					PrintTaskList((T_TaskDescriptor *)HiPriProcList, "hi");
					PrintTaskList((T_TaskDescriptor *)MediumPriProcList, "medium");
					PrintTaskList((T_TaskDescriptor *)LowPriProcList, "low");
					PrintTaskList((T_TaskDescriptor *)BkGroundProcList, "background");
					free(T);
				} else {
					PRINT("Malloc fail;\n\r")
				}
				break;
			case 'D': case 'd':
				{
					PRINT(HOME CLRSCR "Timer delay status:\r\n")
					PRINT("Task ");
					for(unsigned int i=0; i<NUM_TIMERS; i++) PRINT ("T%02i ", i);
					PRINT("\n\r");
					for(unsigned int i=0; i<NOISE_TIMER_TASK_COUNT; i++){
						PRINT (" %02i  ", i);
						TaskTO_Check[i]->PrintCntTimerStatus();
					}
					#if TIMER_NUMBER_CHECK
						PRINT(" - Num of running timers: %i\n\r", NumberOfActiveTimers);
					#endif
					T_TimerStatus T=CheckTimerStatus();
					PRINT(" - Timer status: ")
					switch(T){
						case TimerQueOk: PRINT("ok.\n\r"); break;
						case TimerNumberError: PRINT("number error.\n\r"); break;
						case TimerSequenceError: PRINT("sequence error.\n\r"); break;
						case TimerExpiredInQue: PRINT("timer expired in que.\n\r"); break;
						default: PRINT("unknow.\n\r"); break;
					}
				}
				break;
			case 'R': case 'r':
				{
					DWORD RandomValue=RTK_TestRandom();
					PRINT(HOME CLRSCR "Random status:\r\n")
					PRINT(" - Random value: %lu / 0x%08lX\n\r",
					      (unsigned long)RandomValue, (unsigned long)RandomValue)
				}
				break;
		}
		c=GetCh();
	} while((c!='Q')&&(c!='q'));
	*Terminato=true;
}

/*
					RTK_TestConsolle

	Purpose:
		Run the RTK concurrent console test task and keep the memory stress worker tasks active until the console task ends.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called as the main RTK test task after the scheduler starts.
	Input:
		None.
	Output:
		None.
	Notes:
		The routine starts the status printer task and the memory-check worker tasks. It terminates when PrintStatusTask
		ends on operator request and sets TestConsolleTerminata, then requests an orderly stop of the memory-check workers.
*/
void RTK_TestConsolle(void){
	volatile bool TestConsolleTerminata=false;
	PrintStatusTaskHND=CreateNamedParTask(PrintStatusTask, (DWORD)(&TestConsolleTerminata), RTK_Pack("CONSOLLE TASK"), 4000, TaskPriorityHi);

	T_TaskRandomSched *TaskRandomSched=new T_TaskRandomSched();
	if(!TaskRandomSched->Run("RANDOM SCHED", NOISE_MEMORY_STACK_WORDS, TaskPriorityCritical)) {
		delete TaskRandomSched;
	}
	SCHEDULE

	for(BYTE TaskId=0; TaskId<NOISE_MEMORY_TASK_COUNT; TaskId++) {
		T_TaskMemoryCheck *Task=new T_TaskMemoryCheck(TaskId);
		if(!Task->Run("NOISE MM TASK", NOISE_MEMORY_STACK_WORDS, TaskPriorityHi)) {
			delete Task;
		}
		SCHEDULE
	}

	for(BYTE TaskId=0; TaskId<NOISE_TIMER_TASK_COUNT; TaskId++) {
		TaskTO_Check[TaskId]=new T_TaskTO_Check();
		if(!TaskTO_Check[TaskId]->Run("NOISE TMR TASK", NOISE_MEMORY_STACK_WORDS, TaskPriorityHi)) {
			delete TaskTO_Check[TaskId];
		}
		SCHEDULE
	}

	for(BYTE TaskId=0; TaskId<NOISE_FLOAT_TASK_COUNT; TaskId++) {
		T_FloatNoise *Task=new T_FloatNoise(TaskId);
		if(!Task->Run("NOISE FPU TASK", NOISE_MEMORY_STACK_WORDS, TaskPriorityHi)) {
			delete Task;
		}
		SCHEDULE
	}

	while(!TestConsolleTerminata){
		// Keep the console test active while worker tasks create and release memory and timers.
	}
	// Request an orderly shutdown of all worker tasks created by this test.
	T_RTK_Noise::EndNoise();
}
