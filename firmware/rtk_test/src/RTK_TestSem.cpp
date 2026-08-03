#include "RTK_TestSem.h"
#include "RTK_TestDiag.h"
#include "RTK_TestBoard.h"
#include "RTK.h"

#define SEM_TEST_CYCLES 8U
#define SEM_TEST_PERIOD_TICKS 5U
#define SEM_TEST_GUARD_TICKS 100U
#define SEM_TEST_STACK_WORDS 384U

static bool RunBinarySemaphoreTest(void);
static bool RunCountingSemaphoreTest(void);
static void BinarySemWaiterTask(DWORD SemPar, DWORD CountPar, DWORD DonePar, DWORD UnusedPar);
static void CountingSemWaiterTask(DWORD SemPar, DWORD CountPar, DWORD DonePar, DWORD UnusedPar);

void RTK_RunSemaphoreTests(void) {
    RTK_TestLog("GROUP SEM START");

    if(RunBinarySemaphoreTest()) {
        RTK_TestPass("SEM-001");
    } else {
        RTK_TestFail("SEM-001", "binary semaphore sequence mismatch");
    }

    if(RunCountingSemaphoreTest()) {
        RTK_TestPass("SEM-002");
    } else {
        RTK_TestFail("SEM-002", "counting semaphore sequence mismatch");
    }
}

/*
					BinarySemWaiterTask

	da controllare
		Attende ciclicamente il semaforo binario passato dalla task principale e incrementa il contatore condiviso quando riesce
		ad acquisirlo. Il rilascio resta a carico della task principale.
*/
static void BinarySemWaiterTask(DWORD SemPar, DWORD CountPar, DWORD DonePar, DWORD UnusedPar) {
    Semaphore *Sem=(Semaphore *)SemPar;
    volatile DWORD *WaitCount=(volatile DWORD *)CountPar;
    volatile Flag *WaiterDone=(volatile Flag *)DonePar;
    (void)UnusedPar;

    for(DWORD Cycle=0; Cycle<SEM_TEST_CYCLES; Cycle++) {
        CheckAndWaitForSem(Sem);
        (*WaitCount)++;
    }

    *WaiterDone=true;
}

/*
					CountingSemWaiterTask

	da controllare
		Attende ciclicamente il counting semaphore passato dalla task principale e incrementa il contatore condiviso quando riesce
		ad acquisire un credito.
*/
static void CountingSemWaiterTask(DWORD SemPar, DWORD CountPar, DWORD DonePar, DWORD UnusedPar) {
    T_CountingSem *Sem=(T_CountingSem *)SemPar;
    volatile DWORD *WaitCount=(volatile DWORD *)CountPar;
    volatile Flag *WaiterDone=(volatile Flag *)DonePar;
    (void)UnusedPar;

    for(DWORD Cycle=0; Cycle<SEM_TEST_CYCLES; Cycle++) {
        CheckAndWaitForCountingSem(Sem);
        (*WaitCount)++;
    }

    *WaiterDone=true;
}

/*
					RunBinarySemaphoreTest

	da controllare
		Verifica prima acquisizione e rilascio immediati del semaforo binario, poi controlla che una task in wait venga risvegliata
		una sola volta per ogni Release eseguita dalla task principale.
*/
static bool RunBinarySemaphoreTest(void) {
    Semaphore Sem=SEM_FREE;
    volatile DWORD WaitCount=0;
    volatile Flag WaiterDone=false;

    if(!TestAndSet(&Sem)) {
        RTK_TestPrintf("Binary semaphore initial acquire failed");
        return false;
    }
    if(TestAndSet(&Sem)) {
        RTK_TestPrintf("Binary semaphore double acquire succeeded");
        return false;
    }

    Release(&Sem);
    Sem=SEM_LOCKED;

    T_TaskDescriptor *WaiterTask=CreateNamedMultiParsTask(BinarySemWaiterTask, RTK_Pack("BSEM       "),
                                                         SEM_TEST_STACK_WORDS, TaskPriorityHi,
                                                         (DWORD)&Sem, (DWORD)&WaitCount,
                                                         (DWORD)&WaiterDone, 0U);
    if(WaiterTask==NULL) {
        RTK_TestPrintf("Binary semaphore waiter task creation failed");
        return false;
    }

    for(DWORD Cycle=0; Cycle<SEM_TEST_CYCLES; Cycle++) {
        WaitForTime(SEM_TEST_PERIOD_TICKS);
        if(WaitCount!=Cycle) {
            RTK_TestPrintf("Binary semaphore count changed before release: count=%lu cycle=%lu",
                           (unsigned long)WaitCount, (unsigned long)Cycle);
            if(!WaiterDone) KillTask(WaiterTask);
            return false;
        }

        Release(&Sem);
        DWORD Start=RTK_TestTimestamp();
        while(WaitCount!=(Cycle+1U)) {
            if((RTK_TestTimestamp()-Start)>SEM_TEST_GUARD_TICKS) {
                RTK_TestPrintf("Binary semaphore guard expired: count=%lu cycle=%lu",
                               (unsigned long)WaitCount, (unsigned long)Cycle);
                if(!WaiterDone) KillTask(WaiterTask);
                return false;
            }
            WaitForTime(1U);
        }
    }

    return WaiterDone && WaitCount==SEM_TEST_CYCLES;
}

/*
					RunCountingSemaphoreTest

	da controllare
		Verifica il consumo immediato dei crediti già disponibili e poi controlla che una task in wait venga risvegliata una sola
		volta per ogni PutCountingSem eseguita dalla task principale.
*/
static bool RunCountingSemaphoreTest(void) {
    T_CountingSem Sem=2U;
    volatile DWORD WaitCount=0;
    volatile Flag WaiterDone=false;

    if(!GetCountingSem(&Sem) || !GetCountingSem(&Sem) || GetCountingSem(&Sem)) {
        RTK_TestPrintf("Counting semaphore immediate credit check failed");
        return false;
    }

    T_TaskDescriptor *WaiterTask=CreateNamedMultiParsTask(CountingSemWaiterTask, RTK_Pack("CSEM       "),
                                                         SEM_TEST_STACK_WORDS, TaskPriorityHi,
                                                         (DWORD)&Sem, (DWORD)&WaitCount,
                                                         (DWORD)&WaiterDone, 0U);
    if(WaiterTask==NULL) {
        RTK_TestPrintf("Counting semaphore waiter task creation failed");
        return false;
    }

    for(DWORD Cycle=0; Cycle<SEM_TEST_CYCLES; Cycle++) {
        WaitForTime(SEM_TEST_PERIOD_TICKS);
        if(WaitCount!=Cycle) {
            RTK_TestPrintf("Counting semaphore count changed before put: count=%lu cycle=%lu",
                           (unsigned long)WaitCount, (unsigned long)Cycle);
            if(!WaiterDone) KillTask(WaiterTask);
            return false;
        }

        PutCountingSem(&Sem);
        DWORD Start=RTK_TestTimestamp();
        while(WaitCount!=(Cycle+1U)) {
            if((RTK_TestTimestamp()-Start)>SEM_TEST_GUARD_TICKS) {
                RTK_TestPrintf("Counting semaphore guard expired: count=%lu cycle=%lu",
                               (unsigned long)WaitCount, (unsigned long)Cycle);
                if(!WaiterDone) KillTask(WaiterTask);
                return false;
            }
            WaitForTime(1U);
        }
    }

    return WaiterDone && WaitCount==SEM_TEST_CYCLES && Sem==0U;
}
