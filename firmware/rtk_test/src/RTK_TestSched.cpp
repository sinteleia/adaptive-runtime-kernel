#include "RTK_TestSched.h"
#include "RTK_TestDiag.h"
#include "RTK_TestMain.h"
#include "RTK.h"
#include "MM.H"

#include <string.h>

#define SCHED_PRIO_LEVELS              5
#define SCHED_TASKS_PER_PRIORITY       3
#define SCHED_PRIO_TASK_COUNT          (SCHED_PRIO_LEVELS * SCHED_TASKS_PER_PRIORITY)
#define SCHED_PRIO_TOGGLE_COUNT        30
#define SCHED_PRIO_RECORD_COUNT        (SCHED_PRIO_TASK_COUNT * SCHED_PRIO_TOGGLE_COUNT)
#define SCHED_PRIO_STACK_WORDS         384
#define SCHED_PRIO_WAIT_TICKS          10
#define SCHED_IDLE_WAIT_TICKS          20
#define SCHED_PREEMPT_STACK_WORDS      384
#define SCHED_PREEMPT_RECORD_COUNT     3
#define SCHED_TERMINATE_STACK_WORDS    384
#define SCHED_TERMINATE_WAIT_TICKS     20
#define SCHED_LOCKED_PEND_STACK_WORDS  384
#define SCHED_LOCKED_PEND_RECORD_COUNT 4
#define SCHED_LOCKED_TERM_STACK_WORDS  384
#define SCHED_INVERSION_STACK_WORDS    384
#define SCHED_INVERSION_WAIT_TICKS     300
#define SCHED_INVERSION_TOLERANCE_PCT  25

#define SCHED_DEMCR                    (*(volatile DWORD *)0xE000EDFCUL)
#define SCHED_DWT_CTRL                 (*(volatile DWORD *)0xE0001000UL)
#define SCHED_DWT_CYCCNT               (*(volatile DWORD *)0xE0001004UL)
#define SCHED_DEMCR_TRCENA             0x01000000UL
#define SCHED_DWT_CYCCNTENA            0x00000001UL

typedef struct SchedPrioRecord {
	BYTE TaskId;
	BYTE Priority;
	WORD Toggle;
	DWORD Time;
	DWORD Cycles;
} SchedPrioRecord;

typedef struct SchedHeapSnapshot {
	T_Len HeapDimension;
	T_Len MaxBlockDimension;
	int NumOfFreeBlock;
	int NumOfAllocatedBlock;
} SchedHeapSnapshot;

extern volatile uint32_t uwTick;

static volatile Flag SchedPrioFlag;
static volatile Flag SchedPrioDoneFlag;
static volatile WORD SchedPrioToggle;
static volatile Flag SchedPrioOpenToggle;
static volatile BYTE SchedPrioToggleRecordCount;
static volatile BYTE SchedPrioToggleCloseCount;
static volatile DWORD SchedPrioLogIndex;
static volatile SchedPrioRecord *SchedPrioLog;
static T_TaskDescriptor *SchedPrioTasks[SCHED_PRIO_TASK_COUNT];
static HANDLE SchedPrioTicHandle;
static volatile Flag SchedPreemptReleaseFlag;
static volatile Flag SchedPreemptDoneFlag;
static volatile BYTE SchedPreemptRecordIndex;
static volatile BYTE SchedPreemptRecords[SCHED_PREEMPT_RECORD_COUNT];
static T_TaskDescriptor *SchedPreemptHighTask_hnd;
static T_TaskDescriptor *SchedPreemptLowTask_hnd;
static volatile Flag SchedTerminateDoneFlag;
static volatile DWORD SchedTerminateRunCount;
static T_TaskDescriptor *SchedTerminateTask_hnd;
static volatile Flag SchedLockedPendReleaseFlag;
static volatile Flag SchedLockedPendDoneFlag;
static volatile BYTE SchedLockedPendRecordIndex;
static volatile BYTE SchedLockedPendRecords[SCHED_LOCKED_PEND_RECORD_COUNT];
static T_TaskDescriptor *SchedLockedPendHighTask_hnd;
static T_TaskDescriptor *SchedLockedPendLowTask_hnd;
static volatile DWORD SchedLockedTerminateRunCount;
static T_TaskDescriptor *SchedLockedTerminateTask_hnd;

static void SchedPrioWorker(DWORD TaskId);
static void SchedPrioTicRoutine(void);
static void SchedPreemptHighTask(void);
static void SchedPreemptLowTask(void);
static void SchedTerminateTask(void);
static void SchedLockedPendHighTask(void);
static void SchedLockedPendLowTask(void);
static void SchedLockedTerminateTask(void);
static void SchedInversionCounterTask(DWORD CounterPar);
static bool RunPriorityOrderTest(void);
static bool RunIdleTaskTest(void);
static bool RunPreemptionTest(void);
static bool RunTaskTerminationTest(void);
static bool RunLockedPendSvTest(void);
static bool RunLockedTerminateTest(void);
static bool RunPriorityInversionRatioTest(void);
static void PrintSchedInversionCounter(const char *Name, QWORD Value);
static bool VerifyPriorityOrderLog(void);
static void CleanupPriorityOrderTest(void);
static bool CreateSchedPrioTask(BYTE TaskId, T_TaskPriority Priority);
static T_Text MakeSchedTaskName(BYTE TaskId);
static void RecordPreemptStep(BYTE Step);
static void CleanupPreemptionTest(void);
static void RecordLockedPendStep(BYTE Step);
static void CleanupLockedPendSvTest(void);
static bool CaptureHeapSnapshot(SchedHeapSnapshot *Snapshot);
static bool HeapSnapshotsMatch(const SchedHeapSnapshot *A, const SchedHeapSnapshot *B);
static void EnableCycleCounter(void);
static DWORD ReadCycleCounter(void);

void RTK_RunSchedulerTests(void) {
    RTK_TestLog("GROUP SCHED START");

	if(RunPriorityOrderTest()) {
		RTK_TestPass("SCHED-001");
		RTK_TestPass("SCHED-002");
	} else {
		RTK_TestFail("SCHED-001", "priority order log check failed");
		RTK_TestFail("SCHED-002", "same-priority task coverage check failed");
	}

	if(RunIdleTaskTest()) {
		RTK_TestPass("SCHED-003");
	} else {
		RTK_TestFail("SCHED-003", "idle task did not run while application tasks were waiting");
	}

	if(RunPreemptionTest()) {
		RTK_TestPass("SCHED-005");
	} else {
		RTK_TestFail("SCHED-005", "waiting high-priority task did not preempt lower-priority work");
	}

	if(RunTaskTerminationTest()) {
		RTK_TestPass("SCHED-006");
	} else {
		RTK_TestFail("SCHED-006", "current task termination did not restore heap state");
	}

	if(RunLockedPendSvTest()) {
		RTK_TestPass("SCHED-007");
	} else {
		RTK_TestFail("SCHED-007", "PendSV requested while locked was not served after unlock");
	}

	if(RunLockedTerminateTest()) {
		RTK_TestPass("SCHED-011");
	} else {
		RTK_TestFail("SCHED-011", "Terminate did not restore scheduling after scheduler lock");
	}

	if(RunPriorityInversionRatioTest()) {
		RTK_TestPass("SCHED-012");
	} else {
		RTK_TestFail("SCHED-012", "medium/low priority inversion ratio mismatch");
	}
}

/*
					SchedInversionCounterTask

	da controllare
		Incrementa il contatore passato per puntatore. La stessa funzione viene usata sia dalla task medium sia dalla task low, in
		modo che il rapporto misurato dipenda dalla schedulazione e non dal codice eseguito dalle due task.
*/
static void SchedInversionCounterTask(DWORD CounterPar) {
	volatile QWORD *Counter=(volatile QWORD *)CounterPar;

	for(;;) {
		(*Counter)++;
	}
}

/*
					RunPriorityInversionRatioTest

	da controllare
		Crea una task medium ed una low che incrementano due contatori locali passati per puntatore. Dopo un tempo fisso verifica
		che il rapporto fra i contatori sia compatibile con il parametro RTK_TEST_MEDIUM_FOR_LOW.
*/
static bool RunPriorityInversionRatioTest(void) {
	volatile QWORD MediumCounter=0;
	volatile QWORD LowCounter=0;
	T_TaskDescriptor *MediumTask;
	T_TaskDescriptor *LowTask;

	MediumTask=CreateNamedParTask(SchedInversionCounterTask, (DWORD)&MediumCounter,
	                              RTK_Pack("SIM         "), SCHED_INVERSION_STACK_WORDS,
	                              TaskPriorityMedium);
	if(MediumTask==NULL) {
		RTK_TestPrintf("Scheduler inversion medium task creation failed\n");
		return false;
	}

	LowTask=CreateNamedParTask(SchedInversionCounterTask, (DWORD)&LowCounter,
	                           RTK_Pack("SIL         "), SCHED_INVERSION_STACK_WORDS,
	                           TaskPriorityLow);
	if(LowTask==NULL) {
		RTK_TestPrintf("Scheduler inversion low task creation failed\n");
		KillTask(MediumTask);
		return false;
	}

	WaitForTime(SCHED_INVERSION_WAIT_TICKS);

	KillTask(MediumTask);
	KillTask(LowTask);

	if(MediumCounter==0 || LowCounter==0) {
		RTK_TestPrintf("Scheduler inversion counter not updated\n");
		return false;
	}

	QWORD ScaledMedium=MediumCounter * 100ULL;
	QWORD Expected=LowCounter * (QWORD)RTK_TEST_MEDIUM_FOR_LOW * 100ULL;
	QWORD Min=Expected * (100ULL - SCHED_INVERSION_TOLERANCE_PCT) / 100ULL;
	QWORD Max=Expected * (100ULL + SCHED_INVERSION_TOLERANCE_PCT) / 100ULL;

	if(ScaledMedium<Min || ScaledMedium>Max) {
		RTK_TestPrintf("Scheduler inversion ratio outside tolerance\n");
		PrintSchedInversionCounter("medium", MediumCounter);
		PrintSchedInversionCounter("low", LowCounter);
		PrintSchedInversionCounter("scaled medium", ScaledMedium);
		PrintSchedInversionCounter("expected", Expected);
		PrintSchedInversionCounter("min", Min);
		PrintSchedInversionCounter("max", Max);
		return false;
	}

	return true;
}

/*
					PrintSchedInversionCounter

	da controllare
		Stampa un valore QWORD come due DWORD, evitando di dipendere dal supporto della printf embedded per il formato long long.
*/
static void PrintSchedInversionCounter(const char *Name, QWORD Value) {
	RTK_TestPrintf("Scheduler inversion %s: hi=0x%lx lo=0x%lx\n",
	               Name, (DWORD)(Value>>32), (DWORD)Value);
}

static bool RunIdleTaskTest(void) {
	RTK_TestIdleCounter=0;
	DWORD InitialIdleCounter=RTK_TestIdleCounter;

	WaitForTime(SCHED_IDLE_WAIT_TICKS);

	DWORD FinalIdleCounter=RTK_TestIdleCounter;
	if(FinalIdleCounter==InitialIdleCounter) {
		RTK_TestPrintf("Scheduler idle counter did not change: %lu\n", FinalIdleCounter);
		return false;
	}

	return true;
}

static bool RunPreemptionTest(void) {
	SchedPreemptReleaseFlag=false;
	SchedPreemptDoneFlag=false;
	SchedPreemptRecordIndex=0;
	memset((void *)SchedPreemptRecords, 0, sizeof(SchedPreemptRecords));
	SchedPreemptHighTask_hnd=NULL;
	SchedPreemptLowTask_hnd=NULL;

	SchedPreemptHighTask_hnd=CreateNamedTask(SchedPreemptHighTask, RTK_Pack("SPH         "),
	                                        SCHED_PREEMPT_STACK_WORDS, TaskPriorityHi);
	if(SchedPreemptHighTask_hnd==NULL) {
		RTK_TestPrintf("Scheduler preemption high task creation failed\n");
		CleanupPreemptionTest();
		return false;
	}

	SchedPreemptLowTask_hnd=CreateNamedTask(SchedPreemptLowTask, RTK_Pack("SPL         "),
	                                       SCHED_PREEMPT_STACK_WORDS, TaskPriorityLow);
	if(SchedPreemptLowTask_hnd==NULL) {
		RTK_TestPrintf("Scheduler preemption low task creation failed\n");
		CleanupPreemptionTest();
		return false;
	}

	SCHEDULE;
	WaitForFlag((Flag *)&SchedPreemptDoneFlag);

	bool Result=(SchedPreemptRecordIndex>=2) &&
	            (SchedPreemptRecords[0]==1) &&
	            (SchedPreemptRecords[1]==2);
	if(!Result) {
		RTK_TestPrintf("Scheduler preemption order mismatch: count=%u first=%u second=%u\n",
		               SchedPreemptRecordIndex, SchedPreemptRecords[0], SchedPreemptRecords[1]);
	}

	CleanupPreemptionTest();
	return Result;
}

static void SchedPreemptHighTask(void) {
	WaitForFlag((Flag *)&SchedPreemptReleaseFlag);
	RecordPreemptStep(2);
	SchedPreemptDoneFlag=true;
	SCHEDULE;
	WaitForever();
}

static void SchedPreemptLowTask(void) {
	RecordPreemptStep(1);
	SchedPreemptReleaseFlag=true;
	SCHEDULE;
	RecordPreemptStep(3);
	WaitForever();
}

static void RecordPreemptStep(BYTE Step) {
	uint32_t SchedulerLock=RTK_SchedulerLock();
	if(SchedPreemptRecordIndex<SCHED_PREEMPT_RECORD_COUNT) {
		SchedPreemptRecords[SchedPreemptRecordIndex]=Step;
		SchedPreemptRecordIndex++;
	}
	RTK_Unlock(SchedulerLock);
}

static void CleanupPreemptionTest(void) {
	if(SchedPreemptHighTask_hnd!=NULL) {
		KillTask(SchedPreemptHighTask_hnd);
		SchedPreemptHighTask_hnd=NULL;
	}
	if(SchedPreemptLowTask_hnd!=NULL) {
		KillTask(SchedPreemptLowTask_hnd);
		SchedPreemptLowTask_hnd=NULL;
	}
}

static bool RunTaskTerminationTest(void) {
	SchedHeapSnapshot HeapBefore;
	SchedHeapSnapshot HeapAfter;

	SchedTerminateDoneFlag=false;
	SchedTerminateRunCount=0;
	SchedTerminateTask_hnd=NULL;

	if(!CaptureHeapSnapshot(&HeapBefore)) {
		return false;
	}

	SchedTerminateTask_hnd=CreateNamedTask(SchedTerminateTask, RTK_Pack("STT         "),
	                                      SCHED_TERMINATE_STACK_WORDS, TaskPriorityHi);
	if(SchedTerminateTask_hnd==NULL) {
		RTK_TestPrintf("Scheduler termination task creation failed\n");
		return false;
	}

	SCHEDULE;
	if(!WaitForFlagTO((Flag *)&SchedTerminateDoneFlag, SCHED_TERMINATE_WAIT_TICKS)) {
		RTK_TestPrintf("Scheduler termination task timeout\n");
		if(SchedTerminateTask_hnd!=NULL) {
			KillTask(SchedTerminateTask_hnd);
			SchedTerminateTask_hnd=NULL;
		}
		return false;
	}
	SchedTerminateTask_hnd=NULL;

	if(!CaptureHeapSnapshot(&HeapAfter)) {
		return false;
	}

	if(SchedTerminateRunCount!=1) {
		RTK_TestPrintf("Scheduler termination run count mismatch: %lu\n", SchedTerminateRunCount);
		return false;
	}

	if(!HeapSnapshotsMatch(&HeapBefore, &HeapAfter)) {
		RTK_TestPrintf("Scheduler termination heap mismatch: heap %lu/%lu max %lu/%lu free %d/%d alloc %d/%d\n",
		               (unsigned long)HeapBefore.HeapDimension,
		               (unsigned long)HeapAfter.HeapDimension,
		               (unsigned long)HeapBefore.MaxBlockDimension,
		               (unsigned long)HeapAfter.MaxBlockDimension,
		               HeapBefore.NumOfFreeBlock,
		               HeapAfter.NumOfFreeBlock,
		               HeapBefore.NumOfAllocatedBlock,
		               HeapAfter.NumOfAllocatedBlock);
		return false;
	}

	return true;
}

static void SchedTerminateTask(void) {
	SchedTerminateRunCount++;
	SchedTerminateDoneFlag=true;
}

static bool RunLockedPendSvTest(void) {
	SchedLockedPendReleaseFlag=false;
	SchedLockedPendDoneFlag=false;
	SchedLockedPendRecordIndex=0;
	memset((void *)SchedLockedPendRecords, 0, sizeof(SchedLockedPendRecords));
	SchedLockedPendHighTask_hnd=NULL;
	SchedLockedPendLowTask_hnd=NULL;

	SchedLockedPendHighTask_hnd=CreateNamedTask(SchedLockedPendHighTask, RTK_Pack("LPH         "),
	                                           SCHED_LOCKED_PEND_STACK_WORDS, TaskPriorityHi);
	if(SchedLockedPendHighTask_hnd==NULL) {
		RTK_TestPrintf("Scheduler locked PendSV high task creation failed\n");
		CleanupLockedPendSvTest();
		return false;
	}

	SchedLockedPendLowTask_hnd=CreateNamedTask(SchedLockedPendLowTask, RTK_Pack("LPL         "),
	                                          SCHED_LOCKED_PEND_STACK_WORDS, TaskPriorityLow);
	if(SchedLockedPendLowTask_hnd==NULL) {
		RTK_TestPrintf("Scheduler locked PendSV low task creation failed\n");
		CleanupLockedPendSvTest();
		return false;
	}

	SCHEDULE;
	WaitForFlag((Flag *)&SchedLockedPendDoneFlag);

	bool Result=(SchedLockedPendRecordIndex>=3) &&
	            (SchedLockedPendRecords[0]==1) &&
	            (SchedLockedPendRecords[1]==2) &&
	            (SchedLockedPendRecords[2]==3);
	if(!Result) {
		RTK_TestPrintf("Scheduler locked PendSV order mismatch: count=%u first=%u second=%u third=%u\n",
		               SchedLockedPendRecordIndex,
		               SchedLockedPendRecords[0],
		               SchedLockedPendRecords[1],
		               SchedLockedPendRecords[2]);
	}

	CleanupLockedPendSvTest();
	return Result;
}

static void SchedLockedPendHighTask(void) {
	WaitForFlag((Flag *)&SchedLockedPendReleaseFlag);
	RecordLockedPendStep(3);
	SchedLockedPendDoneFlag=true;
	SCHEDULE;
	WaitForever();
}

static void SchedLockedPendLowTask(void) {
	RecordLockedPendStep(1);
	uint32_t SchedulerLock=RTK_SchedulerLock();
	SchedLockedPendReleaseFlag=true;
	SCHEDULE;
	RecordLockedPendStep(2);
	RTK_Unlock(SchedulerLock);
	RecordLockedPendStep(4);
	WaitForever();
}

static void RecordLockedPendStep(BYTE Step) {
	uint32_t SchedulerLock=RTK_SchedulerLock();
	if(SchedLockedPendRecordIndex<SCHED_LOCKED_PEND_RECORD_COUNT) {
		SchedLockedPendRecords[SchedLockedPendRecordIndex]=Step;
		SchedLockedPendRecordIndex++;
	}
	RTK_Unlock(SchedulerLock);
}

static void CleanupLockedPendSvTest(void) {
	if(SchedLockedPendHighTask_hnd!=NULL) {
		KillTask(SchedLockedPendHighTask_hnd);
		SchedLockedPendHighTask_hnd=NULL;
	}
	if(SchedLockedPendLowTask_hnd!=NULL) {
		KillTask(SchedLockedPendLowTask_hnd);
		SchedLockedPendLowTask_hnd=NULL;
	}
}

static bool RunLockedTerminateTest(void) {
	SchedHeapSnapshot HeapBefore;
	SchedHeapSnapshot HeapAfter;

	SchedLockedTerminateRunCount=0;
	SchedLockedTerminateTask_hnd=NULL;

	if(!CaptureHeapSnapshot(&HeapBefore)) {
		return false;
	}

	SchedLockedTerminateTask_hnd=CreateNamedTask(SchedLockedTerminateTask, RTK_Pack("SLT         "),
	                                            SCHED_LOCKED_TERM_STACK_WORDS, TaskPriorityHi);
	if(SchedLockedTerminateTask_hnd==NULL) {
		RTK_TestPrintf("Scheduler locked terminate task creation failed\n");
		return false;
	}

	SCHEDULE;
	SchedLockedTerminateTask_hnd=NULL;

	if(!CaptureHeapSnapshot(&HeapAfter)) {
		return false;
	}

	if(SchedLockedTerminateRunCount!=1) {
		RTK_TestPrintf("Scheduler locked terminate run count mismatch: %lu\n",
		               SchedLockedTerminateRunCount);
		return false;
	}

	if(!HeapSnapshotsMatch(&HeapBefore, &HeapAfter)) {
		RTK_TestPrintf("Scheduler locked terminate heap mismatch: heap %lu/%lu max %lu/%lu free %d/%d alloc %d/%d\n",
		               (unsigned long)HeapBefore.HeapDimension,
		               (unsigned long)HeapAfter.HeapDimension,
		               (unsigned long)HeapBefore.MaxBlockDimension,
		               (unsigned long)HeapAfter.MaxBlockDimension,
		               HeapBefore.NumOfFreeBlock,
		               HeapAfter.NumOfFreeBlock,
		               HeapBefore.NumOfAllocatedBlock,
		               HeapAfter.NumOfAllocatedBlock);
		return false;
	}

	return true;
}

static void SchedLockedTerminateTask(void) {
	SchedLockedTerminateRunCount++;
	(void)RTK_SchedulerLock();
}

static bool CaptureHeapSnapshot(SchedHeapSnapshot *Snapshot) {
	T_HeapStatus HeapStatusResult=HeapStatus(&Snapshot->HeapDimension,
	                                        &Snapshot->MaxBlockDimension,
	                                        &Snapshot->NumOfFreeBlock,
	                                        &Snapshot->NumOfAllocatedBlock);
	if(HeapStatusResult!=HeapOk) {
		RTK_TestPrintf("Scheduler heap status failed: %d\n", HeapStatusResult);
		return false;
	}

	return true;
}

static bool HeapSnapshotsMatch(const SchedHeapSnapshot *A, const SchedHeapSnapshot *B) {
	return (A->HeapDimension==B->HeapDimension) &&
	       (A->MaxBlockDimension==B->MaxBlockDimension) &&
	       (A->NumOfFreeBlock==B->NumOfFreeBlock) &&
	       (A->NumOfAllocatedBlock==B->NumOfAllocatedBlock);
}

static void SchedPrioWorker(DWORD TaskId) {
	WORD LastRecordedToggle=0xFFFFU;
	WORD LastClosedToggle=0xFFFFU;

	for(WORD Toggle=0; Toggle<SCHED_PRIO_TOGGLE_COUNT; Toggle++) {
		WaitForFlag((Flag *)&SchedPrioFlag);

		uint32_t SchedulerLock=RTK_SchedulerLock();
		WORD CurrentToggle=SchedPrioToggle;
		DWORD RecordIndex=SchedPrioLogIndex;
		bool RecordThisToggle=(CurrentToggle<SCHED_PRIO_TOGGLE_COUNT) &&
		                      (CurrentToggle!=LastRecordedToggle) &&
		                      (SchedPrioLog!=NULL) &&
		                      (SchedPrioLogIndex<SCHED_PRIO_RECORD_COUNT);
		if(RecordThisToggle) {
			SchedPrioLogIndex++;
		}
		RTK_Unlock(SchedulerLock);

		if(RecordThisToggle) {
			SchedPrioLog[RecordIndex].TaskId=(BYTE)TaskId;
			SchedPrioLog[RecordIndex].Priority=((T_TaskDescriptor *)CurrentTaskPtr)->TaskPriority;
			SchedPrioLog[RecordIndex].Toggle=CurrentToggle;
			SchedPrioLog[RecordIndex].Time=uwTick;
			SchedPrioLog[RecordIndex].Cycles=ReadCycleCounter();

			SchedulerLock=RTK_SchedulerLock();
			LastRecordedToggle=CurrentToggle;
			if(CurrentToggle==SchedPrioToggle && SchedPrioToggleRecordCount<SCHED_PRIO_TASK_COUNT) {
				SchedPrioToggleRecordCount++;
				if(SchedPrioToggleRecordCount>=SCHED_PRIO_TASK_COUNT) {
					SchedPrioFlag=false;
					SchedPrioToggleRecordCount=0;
					SCHEDULE;
				}
			}
			RTK_Unlock(SchedulerLock);
		}

		WaitForNotFlag((Flag *)&SchedPrioFlag);

		SchedulerLock=RTK_SchedulerLock();
		if(CurrentToggle==SchedPrioToggle && CurrentToggle!=LastClosedToggle) {
			LastClosedToggle=CurrentToggle;
			if(SchedPrioToggleCloseCount<SCHED_PRIO_TASK_COUNT) {
				SchedPrioToggleCloseCount++;
				if(SchedPrioToggleCloseCount>=SCHED_PRIO_TASK_COUNT) {
					SchedPrioToggleCloseCount=0;
					SchedPrioToggle++;
					SchedPrioOpenToggle=false;
					if(SchedPrioToggle>=SCHED_PRIO_TOGGLE_COUNT) {
						SchedPrioDoneFlag=true;
					}
					SCHEDULE;
				}
			}
		}
		RTK_Unlock(SchedulerLock);
	}

	WaitForever();
}

static void SchedPrioTicRoutine(void) {
	if(SchedPrioDoneFlag || SchedPrioToggle>=SCHED_PRIO_TOGGLE_COUNT) {
		return;
	}

	if(!SchedPrioOpenToggle) {
		SchedPrioOpenToggle=true;
		SchedPrioToggleRecordCount=0;
		SchedPrioToggleCloseCount=0;
		SchedPrioFlag=true;
		SCHEDULE;
	}
}

static bool RunPriorityOrderTest(void) {
	memset(SchedPrioTasks, 0, sizeof(SchedPrioTasks));
	SchedPrioLog=(volatile SchedPrioRecord *)malloc(sizeof(SchedPrioRecord) * SCHED_PRIO_RECORD_COUNT);
	if(SchedPrioLog==NULL) {
		RTK_TestPrintf("Scheduler priority log allocation failed\n");
		return false;
	}
	memset((void *)SchedPrioLog, 0, sizeof(SchedPrioRecord) * SCHED_PRIO_RECORD_COUNT);
	SchedPrioFlag=false;
	SchedPrioDoneFlag=false;
	SchedPrioToggle=0;
	SchedPrioOpenToggle=false;
	SchedPrioToggleRecordCount=0;
	SchedPrioToggleCloseCount=0;
	SchedPrioLogIndex=0;
	SchedPrioTicHandle=INVALID_HANDLE;

	EnableCycleCounter();

	if(!CreateSchedPrioTask(0, TaskPriorityCritical)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(1, TaskPriorityCritical)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(2, TaskPriorityCritical)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(3, TaskPriorityHi)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(4, TaskPriorityHi)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(5, TaskPriorityHi)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(6, TaskPriorityMedium)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(7, TaskPriorityMedium)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(8, TaskPriorityMedium)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(9, TaskPriorityLow)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(10, TaskPriorityLow)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(11, TaskPriorityLow)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(12, TaskPriorityBackGround)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(13, TaskPriorityBackGround)) { CleanupPriorityOrderTest(); return false; }
	if(!CreateSchedPrioTask(14, TaskPriorityBackGround)) { CleanupPriorityOrderTest(); return false; }

	SCHEDULE;
	WaitForTime(1);

	SchedPrioTicHandle=AgganciaTic(SchedPrioTicRoutine);
	if(SchedPrioTicHandle==INVALID_HANDLE) {
		RTK_TestPrintf("Scheduler priority tic hook failed\n");
		CleanupPriorityOrderTest();
		return false;
	}
	WaitForFlag((Flag *)&SchedPrioDoneFlag);

	bool Result=VerifyPriorityOrderLog();
	CleanupPriorityOrderTest();
	return Result;
}

static bool VerifyPriorityOrderLog(void) {
	if(SchedPrioLogIndex!=SCHED_PRIO_RECORD_COUNT) {
		RTK_TestPrintf("Scheduler priority log length mismatch: %lu/%u\n", SchedPrioLogIndex, SCHED_PRIO_RECORD_COUNT);
		return false;
	}

	for(WORD Toggle=0; Toggle<SCHED_PRIO_TOGGLE_COUNT; Toggle++) {
		bool SeenMediumOrLow=false;
		bool SeenBackground=false;
		BYTE TaskHit[SCHED_PRIO_TASK_COUNT];
		BYTE PrioHit[SCHED_PRIO_LEVELS];
		DWORD ToggleRecords=0;

		memset(TaskHit, 0, sizeof(TaskHit));
		memset(PrioHit, 0, sizeof(PrioHit));

		for(DWORD RecordIndex=0; RecordIndex<SCHED_PRIO_RECORD_COUNT; RecordIndex++) {
			const volatile SchedPrioRecord *Record=&SchedPrioLog[RecordIndex];
			if(Record->Toggle!=Toggle) {
				continue;
			}

			if(Record->TaskId>=SCHED_PRIO_TASK_COUNT || Record->Priority>=SCHED_PRIO_LEVELS) {
				RTK_TestPrintf("Scheduler priority invalid record at toggle %u record %lu\n", Toggle, RecordIndex);
				return false;
			}

			if(Record->Priority==TaskPriorityMedium || Record->Priority==TaskPriorityLow) {
				SeenMediumOrLow=true;
			}
			if(Record->Priority==TaskPriorityBackGround) {
				SeenBackground=true;
			}
			if(SeenMediumOrLow && Record->Priority<TaskPriorityMedium) {
				RTK_TestPrintf("Scheduler high priority after medium/low at toggle %u record %lu\n", Toggle, RecordIndex);
				return false;
			}
			if(SeenBackground && Record->Priority<TaskPriorityBackGround) {
				RTK_TestPrintf("Scheduler non-background after background at toggle %u record %lu\n", Toggle, RecordIndex);
				return false;
			}

			TaskHit[Record->TaskId]++;
			PrioHit[Record->Priority]++;
			ToggleRecords++;
		}

		if(ToggleRecords!=SCHED_PRIO_TASK_COUNT) {
			RTK_TestPrintf("Scheduler toggle %u record count mismatch: %lu/%u\n",
			               Toggle, ToggleRecords, SCHED_PRIO_TASK_COUNT);
			return false;
		}

		for(BYTE TaskId=0; TaskId<SCHED_PRIO_TASK_COUNT; TaskId++) {
			if(TaskHit[TaskId]!=1) {
				RTK_TestPrintf("Scheduler task %u hit mismatch at toggle %u: %u\n", TaskId, Toggle, TaskHit[TaskId]);
				return false;
			}
		}

		for(BYTE Priority=0; Priority<SCHED_PRIO_LEVELS; Priority++) {
			if(PrioHit[Priority]!=SCHED_TASKS_PER_PRIORITY) {
				RTK_TestPrintf("Scheduler priority %u hit mismatch at toggle %u: %u\n", Priority, Toggle, PrioHit[Priority]);
				return false;
			}
		}
	}

	return true;
}

static void CleanupPriorityOrderTest(void) {
	if(SchedPrioTicHandle!=INVALID_HANDLE) {
		Sgancia(SchedPrioTicHandle);
		SchedPrioTicHandle=INVALID_HANDLE;
	}
	SchedPrioFlag=false;
	SchedPrioDoneFlag=true;
	for(BYTE TaskId=0; TaskId<SCHED_PRIO_TASK_COUNT; TaskId++) {
		if(SchedPrioTasks[TaskId]!=NULL) {
			KillTask(SchedPrioTasks[TaskId]);
			SchedPrioTasks[TaskId]=NULL;
		}
	}
	if(SchedPrioLog!=NULL) {
		free((void *)SchedPrioLog);
		SchedPrioLog=NULL;
	}
	SchedPrioToggleRecordCount=0;
	SchedPrioToggleCloseCount=0;
	SchedPrioOpenToggle=false;
}

static bool CreateSchedPrioTask(BYTE TaskId, T_TaskPriority Priority) {
	SchedPrioTasks[TaskId]=CreateNamedParTask(SchedPrioWorker, TaskId, MakeSchedTaskName(TaskId),
	                                          SCHED_PRIO_STACK_WORDS, Priority);
	return SchedPrioTasks[TaskId]!=NULL;
}

static T_Text MakeSchedTaskName(BYTE TaskId) {
	char Name[]="SCH00       ";
	Name[3]=(char)('0' + (TaskId / 10));
	Name[4]=(char)('0' + (TaskId % 10));
	return RTK_Pack(Name);
}

static void EnableCycleCounter(void) {
	SCHED_DEMCR|=SCHED_DEMCR_TRCENA;
	SCHED_DWT_CYCCNT=0;
	SCHED_DWT_CTRL|=SCHED_DWT_CYCCNTENA;
}

static DWORD ReadCycleCounter(void) {
	return SCHED_DWT_CYCCNT;
}
