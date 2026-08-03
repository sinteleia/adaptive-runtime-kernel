#include "RTK_TestMM.h"
#include "RTK_Noise.h"
#include "RTK_TestDiag.h"
#include "MM.H"

#define MM_TEST_RUN_TICKS 10000U /* Test duration */
#define MM_TEST_WAIT_STEP_TICKS 500U /* Delay for emitting a dot on consolle */
#define MM_TASK_COUNT 10U /* Number of noise task created */

typedef struct MmHeapSnapshot {
	T_Len HeapDimension;
	T_Len MaxBlockDimension;
	int NumOfFreeBlock;
	int NumOfAllocatedBlock;
} MmHeapSnapshot;

static bool RunDynamicAllocationTest(void);
static bool CaptureMemoryHeapSnapshot(MmHeapSnapshot *Snapshot);
static bool MemoryHeapSnapshotsMatch(const MmHeapSnapshot *A, const MmHeapSnapshot *B);
static const char *MemoryHeapStatusName(T_HeapStatus HeapStatusResult);

void RTK_RunMemoryTests(void) {
    RTK_TestLog("GROUP MM START");

	if(RunDynamicAllocationTest()) {
		RTK_TestPass("MM-001");
	} else {
		RTK_TestFail("MM-001", "dynamic allocation stress failed");
	}
}

/*
					RunDynamicAllocationTest
	da controllare

		Porta temporaneamente la task corrente a priorita' Hi, avvia piu' istanze C++ identiche alla stessa priorita' e le lascia
		stressare malloc/free senza wait. Alla scadenza del tempo ripristina la priorita' originale e confronta lo heap finale.
*/
static bool RunDynamicAllocationTest(void) {
	MmHeapSnapshot HeapBefore;
	MmHeapSnapshot HeapAfter;
	T_TaskDescriptor *CurrentTask=(T_TaskDescriptor *)CurrentTaskPtr;
	T_TaskPriority SavedPriority=(T_TaskPriority)CurrentTask->TaskPriority;
	bool Result=false;

	if(!CaptureMemoryHeapSnapshot(&HeapBefore)) return false;
	if(!ChangeTaskPriority(CurrentTask, TaskPriorityHi)) return false;
	if(CurrentTask->TaskPriority!=TaskPriorityHi) goto RestorePriority;

	T_TaskMemoryCheck::ResetTestState();
	for(BYTE TaskId=0; TaskId<MM_TASK_COUNT; TaskId++) {
		T_TaskMemoryCheck *Task=new T_TaskMemoryCheck(TaskId);
		if(Task==NULL) {
			T_TaskMemoryCheck::FailureFlag=true;
			T_TaskMemoryCheck::FailureCode=0x4000UL | TaskId;
			break;
		}
		char Name[]="MM0         ";
		Name[2]=(char)('0' + TaskId);
		if(!Task->Run(Name, NOISE_MEMORY_STACK_WORDS, TaskPriorityHi)) {
			delete Task;
			T_TaskMemoryCheck::FailureFlag=true;
			T_TaskMemoryCheck::FailureCode=0x5000UL | TaskId;
			break;
		}
	}

	for(DWORD RemainingTicks=MM_TEST_RUN_TICKS; RemainingTicks!=0U;) {
		DWORD WaitTicks=(RemainingTicks>MM_TEST_WAIT_STEP_TICKS) ? MM_TEST_WAIT_STEP_TICKS : RemainingTicks;
		WaitForTime(WaitTicks);
		RemainingTicks-=WaitTicks;
		RTK_TestProgress();
	}

	if(!T_RTK_Noise::EndNoise()) goto RestorePriority;

	if(T_TaskMemoryCheck::FailureFlag) {
		RTK_TestPrintf("MM dynamic allocation failure code: 0x%lx", T_TaskMemoryCheck::FailureCode);
		goto RestorePriority;
	}
	if(!CaptureMemoryHeapSnapshot(&HeapAfter)) goto RestorePriority;
	if(!MemoryHeapSnapshotsMatch(&HeapBefore, &HeapAfter)) {
		RTK_TestPrintf("MM heap mismatch: heap %lu/%lu max %lu/%lu free %d/%d alloc %d/%d",
		               (unsigned long)HeapBefore.HeapDimension,
		               (unsigned long)HeapAfter.HeapDimension,
		               (unsigned long)HeapBefore.MaxBlockDimension,
		               (unsigned long)HeapAfter.MaxBlockDimension,
		               HeapBefore.NumOfFreeBlock,
		               HeapAfter.NumOfFreeBlock,
		               HeapBefore.NumOfAllocatedBlock,
		               HeapAfter.NumOfAllocatedBlock);
		goto RestorePriority;
	}
	Result=true;

RestorePriority:
	if(!ChangeTaskPriority(CurrentTask, SavedPriority)) Result=false;
	if(CurrentTask->TaskPriority!=SavedPriority) Result=false;
	return Result;
}

/*
					CaptureMemoryHeapSnapshot
	da controllare

		Acquisisce lo stato diagnostico dell'heap e segnala l'errore se HeapStatus non ritorna HeapOk.
*/
static bool CaptureMemoryHeapSnapshot(MmHeapSnapshot *Snapshot) {
	T_HeapStatus HeapStatusResult=HeapStatus(&Snapshot->HeapDimension,
	                                        &Snapshot->MaxBlockDimension,
	                                        &Snapshot->NumOfFreeBlock,
	                                        &Snapshot->NumOfAllocatedBlock);
	if(HeapStatusResult!=HeapOk) {
		RTK_TestPrintf("MM heap status failed: %d %s heap=%lu max=%lu free=%d alloc=%d",
		               HeapStatusResult,
		               MemoryHeapStatusName(HeapStatusResult),
		               (unsigned long)Snapshot->HeapDimension,
		               (unsigned long)Snapshot->MaxBlockDimension,
		               Snapshot->NumOfFreeBlock,
		               Snapshot->NumOfAllocatedBlock);
		return false;
	}
	return true;
}

/*
					MemoryHeapStatusName
	da controllare

		Converte il codice diagnostico di HeapStatus in testo breve, per rendere leggibile il log seriale dei test memoria.
*/
static const char *MemoryHeapStatusName(T_HeapStatus HeapStatusResult) {
	switch(HeapStatusResult) {
		case HeapOk: return "HeapOk";
		case BlkAddressError: return "BlkAddressError";
		case BlkOutOfHeapMemory: return "BlkOutOfHeapMemory";
		case BlkSizeError: return "BlkSizeError";
		case BlocchiLiberiAdiacenti: return "BlocchiLiberiAdiacenti";
		case BlocchiLiberiSovrapposti: return "BlocchiLiberiSovrapposti";
		case BlockNumberError: return "BlockNumberError";
		case GuardiaSfondata: return "GuardiaSfondata";
		default: return "Unknown";
	}
}

/*
					MemoryHeapSnapshotsMatch
	da controllare

		Confronta due snapshot heap per verificare che il test non abbia lasciato allocazioni o frammentazione residua.
*/
static bool MemoryHeapSnapshotsMatch(const MmHeapSnapshot *A, const MmHeapSnapshot *B) {
	return (A->HeapDimension==B->HeapDimension) &&
	       (A->MaxBlockDimension==B->MaxBlockDimension) &&
	       (A->NumOfFreeBlock==B->NumOfFreeBlock) &&
	       (A->NumOfAllocatedBlock==B->NumOfAllocatedBlock);
}

