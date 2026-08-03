#include <cstdio>
#include "RTK_TestMain.h"
#include "MainTask.h"
#include "RTK_TestBoard.h"
#include "RTK_TestDiag.h"
#include "RTK.h"
#include "Sched.h"
#include "RTK_Interface.h"
#include "MM.H"
#include "RTK_TestConsolle.h"
#include "MicroDelay.h"

#define RTK_TEST_IDLE_STACK_WORDS 10
#define RTK_TEST_MAIN_STACK_WORDS 2048
#define RTK_TEST_TIC_PER_TAU 10
#define RTK_TEST_MAX_SCHED_ROUTINES 8
#define RTK_TEST_MAX_ISR_ROUTINES 4
#define NUM_TEST 3

volatile DWORD RTK_TestIdleCounter;
volatile DWORD RTK_TestMainTaskExitCounter;
static DWORD RTK_TestRandomFallback;

/*
	RTK_TestRandom

	Purpose:
		Return a random-like value for RTK tests when the board does not provide a hardware random source.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Available to RTK test code. Board ports may override this weak routine with a hardware RNG implementation.
	Input:
		None.
	Output:
		Pseudo-random fallback value.
	Notes:
		The weak fallback is intended for test variation only and is not a safety or security random source.
*/
extern "C" __attribute__((weak)) DWORD RTK_TestRandom(void) {
	RTK_TestRandomFallback=(RTK_TestRandomFallback * 1664525UL) + 1013904223UL + RTK_TestTimestamp();
	return RTK_TestRandomFallback;
}

/*
	x

	Purpose:
		Request a scheduler pass from the asynchronous timer used for PendSV scope evidence.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by the RTK test asynchronous timer during the operator-assisted PendSV evidence test.
	Input:
		None.
	Output:
		None. A PendSV scheduling request is generated.
*/
void x(){
	SCHEDULE;
}

/*
	PendSvScopeEvidenceTask

	Purpose:
		Run the operator-assisted PendSV oscilloscope evidence task.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Created by RTK_TestMain() after the automated RTK test cycles.
	Input:
		None.
	Output:
		None. The task records pass/fail evidence for RTK-TC-SCHED-008.
*/
static void PendSvScopeEvidenceTask(void) {
    int OperatorResult;
    RTK_TestDiagSetCompactProgress(false);
    RTK_TestLog("OPERATOR: automated RTK tests complete");
    RTK_TestLog("OPERATOR: async timer enabled for RTK-TC-SCHED-008 scope evidence");
    RTK_TestLog("OPERATOR: verify that a timer pulse inside PendSV is followed immediately by another PendSV pulse");
    RTK_TestLog("OPERATOR: press Y if RTK-TC-SCHED-008 evidence is OK, N otherwise");
    RTK_TestAsyncTimerStart(x, 1999);
   	while(true){
        OperatorResult=GetCh();
        if((OperatorResult=='Y') || (OperatorResult=='y')) {
            RTK_TestAsyncTimerStop();
            RTK_TestPass("SCHED-008");
            break;
        }
        if((OperatorResult=='N') || (OperatorResult=='n')) {
            RTK_TestAsyncTimerStop();
            RTK_TestFail("SCHED-008", "operator rejected oscilloscope evidence");
            break;
        }
    }
    WaitForTime(10);
}

void IdleTask(void) {
    for (;;) {
        RTK_TestIdleCounter++;
    }
}

T_TaskDescriptor *MainTaskHND;

/*
	RTK_TestMain

	Purpose:
		Initialize the RTK test environment and run the complete RTK test campaign.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by the board application after MCU and board startup.
	Input:
		None.
	Output:
		None. Test results are emitted through the configured test console.
	Notes:
		The routine runs repeated scheduler cycles, records heap and scheduler evidence, then starts the interactive console test.
*/
extern "C" void RTK_TestMain(void) {

    RTK_TestBoardInit();
    RTK_TestDiagInit();
	Init_uS_ToDelay();

    // Heap inizialization storing of the initial status 
    minit();
    if(!RTK_TestBoardConsoleInit()) {
        RTK_TestFatal("RTK_TestBoardConsoleInit failed");
        return;
    }
    T_Len InitialHeapDimension;
    T_Len InitialMaxBlockDimension;
    int InitialNumOfFreeBlock;
    int InitialNumOfAllocatedBlock;
   	T_HeapStatus HS=HeapStatus(&InitialHeapDimension, &InitialMaxBlockDimension, &InitialNumOfFreeBlock, &InitialNumOfAllocatedBlock);
    if (HS==HeapOk){
    	printf("Heap memory initialized\n\r");
       	printf("Heap dimension=0x%lx bytes\n\r", (unsigned long)InitialHeapDimension);
       	printf("Max block dimension=0x%lx bytes\n\r", (unsigned long)InitialMaxBlockDimension);
      	printf("Free block number=%i\n\r", InitialNumOfFreeBlock);
      	printf("Allocated block number=%i\n\r", InitialNumOfAllocatedBlock);
    }

    else{
        RTK_TestFatal("Heap memory initialization fail\n\r");
        return;
    }

    if(SchedulerStart()) {
        RTK_TestFail("SCHED-010", "SchedulerStart succeeded before first SchedulerInit");
    } else {
        RTK_TestPass("SCHED-010");
    }

    for(unsigned i=0; i<NUM_TEST; i++){

        RTK_TestDiagBeginCycle(i + 1U);
        DWORD MainTaskExitCounterBefore=RTK_TestMainTaskExitCounter;

        if((i&1U)==0U) {
            if (!SchedulerInit(IdleTask, RTK_TEST_TIC_PER_TAU, RTK_TEST_MEDIUM_FOR_LOW, RTK_TEST_MAX_SCHED_ROUTINES,
                               RTK_TEST_MAX_ISR_ROUTINES)){
                RTK_TestFatal("SchedulerInit failed");
                return;
            }
        }
        MainTaskHND=CreateNamedTask(MainTask, RTK_Pack("RTK TEST    "), RTK_TEST_MAIN_STACK_WORDS, TaskPriorityMedium);
        if (MainTaskHND==NULL){
            RTK_TestFatal("CreateNamedTask(MainTask) failed");
            return;
        }

        if(!SchedulerStart()){
            if(i>0) {
                RTK_TestFail("SCHED-009", ((i&1U)==0U) ?
                             "SchedulerStart failed after repeated SchedulerInit" :
                             "SchedulerStart failed without repeated SchedulerInit");
            }
            RTK_TestFatal("SchedulerStart failed");
            return;
        }
        if(i==1) {
            RTK_TestPass("SCHED-009");
        }
        DWORD MainTaskExitCounterAfter=RTK_TestMainTaskExitCounter;
        if(MainTaskExitCounterAfter==(MainTaskExitCounterBefore + 1)) {
            RTK_TestPass("SCHED-004");
        } else {
            RTK_TestFail("SCHED-004", "main task exit counter did not increment once");
        }

        // Check the actual heap status. Any difference from the initial is rapresentative of an error.
        T_Len ActualHeapDimension;
        T_Len ActualMaxBlockDimension;
        int ActualNumOfFreeBlock;
        int ActualNumOfAllocatedBlock;
        HS=HeapStatus(&ActualHeapDimension, &ActualMaxBlockDimension, &ActualNumOfFreeBlock, &ActualNumOfAllocatedBlock);
        if (HS==HeapOk) {
        }
        else{
            RTK_TestFatal("Heap memory not ok\n\r");
            return;
        }
        bool HeapBaselineMatch=true;
        if(InitialHeapDimension!=ActualHeapDimension){
            printf("warning, HeapDimension differs from the previous: 0x%lx->0x%lx;\n\r",
                   (unsigned long)InitialHeapDimension, (unsigned long)ActualHeapDimension);
            HeapBaselineMatch=false;
            InitialHeapDimension=ActualHeapDimension;
        }
        if(InitialMaxBlockDimension!=ActualMaxBlockDimension){
            printf("warning, Max block dimension differs from the previous: 0x%lx->0x%lx;\n\r",
                   (unsigned long)InitialMaxBlockDimension, (unsigned long)ActualMaxBlockDimension);
            HeapBaselineMatch=false;
            InitialMaxBlockDimension=ActualMaxBlockDimension;
        }
        if(InitialNumOfFreeBlock!=ActualNumOfFreeBlock){
            printf("warning, Num of free block differs from the previous: %i->%i;\n\r", InitialNumOfFreeBlock, ActualNumOfFreeBlock);
            HeapBaselineMatch=false;
            InitialNumOfFreeBlock=ActualNumOfFreeBlock;
        }
        if(InitialNumOfAllocatedBlock!=ActualNumOfAllocatedBlock){
            printf("warning, Num of free block differs from the previous: %i->%i;\n\r", InitialNumOfAllocatedBlock, ActualNumOfAllocatedBlock);
            HeapBaselineMatch=false;
            InitialNumOfAllocatedBlock=ActualNumOfAllocatedBlock;
        }
        if(HeapBaselineMatch) {
            RTK_TestPass("MM-002");
        } else {
            RTK_TestFail("MM-002", "heap baseline changed after scheduler return");
        }
        RTK_TestDiagEndCycle();
    }


    if (!SchedulerInit(IdleTask, RTK_TEST_TIC_PER_TAU, RTK_TEST_MEDIUM_FOR_LOW, RTK_TEST_MAX_SCHED_ROUTINES, RTK_TEST_MAX_ISR_ROUTINES)){
        RTK_TestFatal("SchedulerInit failed before PendSV scope evidence");
        return;
    }

    // PendSV reentrancy test
    MainTaskHND=CreateNamedTask(PendSvScopeEvidenceTask, RTK_Pack("SCOPE EV    "),
                                RTK_TEST_MAIN_STACK_WORDS, TaskPriorityMedium);
    if (MainTaskHND==NULL){
        RTK_TestFatal("CreateNamedTask(PendSvScopeEvidenceTask) failed");
        return;
    }
    if(!SchedulerStart()){
        RTK_TestFatal("SchedulerStart failed before PendSV scope evidence");
        return;
    }

    // Debug interface test
    MainTaskHND=CreateNamedTask(RTK_TestConsolle, RTK_Pack("CONCORRENCE TEST TASK"), RTK_TEST_MAIN_STACK_WORDS, TaskPriorityHi);
    if (MainTaskHND==NULL){
        RTK_TestFatal("CreateNamedTask(RTK_TestConsolle) failed");
        return;
    }
    if(!SchedulerStart()){
        RTK_TestFatal("SchedulerStart failed before RTK_TestConsolle");
        return;
    }

    RTK_TestSummary();
    while(!IsConsolOutputEnded());

}
