#include "RTK_TestWait.h"
#include "RTK_TestDiag.h"
#include "RTK_TestBoard.h"
#include "RTK.h"

#define WAIT_TIME_TEST_TICKS 100U
#define WAIT_TIMEOUT_TEST_TICKS 20U
#define WAIT_FLAG_TEST_CYCLES 8U
#define WAIT_FLAG_TEST_PERIOD_TICKS 5U
#define WAIT_FLAG_TEST_GUARD_TICKS 100U
#define WAIT_FLAG_TEST_STACK_WORDS 384U
#define WAIT_FLAG_MODE_SET 1U
#define WAIT_FLAG_MODE_RESET 0U
#define WAIT_BIT_TEST_CYCLES 8U
#define WAIT_BIT_TEST_PERIOD_TICKS 5U
#define WAIT_BIT_TEST_GUARD_TICKS 100U
#define WAIT_BIT_TEST_STACK_WORDS 384U
#define WAIT_BIT_MODE_BYTE_SET 0U
#define WAIT_BIT_MODE_BYTE_RESET 1U
#define WAIT_BIT_MODE_WORD_SET 2U
#define WAIT_BIT_MODE_WORD_RESET 3U
#define WAIT_BIT_MODE_DWORD_SET 4U
#define WAIT_BIT_MODE_DWORD_RESET 5U
#define WAIT_BIT_TEST_BYTE_BIT 3U
#define WAIT_BIT_TEST_WORD_BIT 11U
#define WAIT_BIT_TEST_DWORD_BIT 27U
#define WAIT_SEM_TEST_CYCLES 8U
#define WAIT_SEM_TEST_PERIOD_TICKS 5U
#define WAIT_SEM_TEST_GUARD_TICKS 100U
#define WAIT_SEM_TEST_STACK_WORDS 384U
#define WAIT_SEM_TEST_TIMEOUT_TICKS 20U
#define WAIT_SEM_MODE_BINARY 0U
#define WAIT_SEM_MODE_COUNTING 1U
#define WAIT_QUE_TEST_SIZE 7U
#define WAIT_QUE_TEST_ITEMS 24U
#define WAIT_QUE_TEST_TIMEOUT_TICKS 20U
#define WAIT_QUE_TEST_GUARD_TICKS 100U
#define WAIT_QUE_TEST_STACK_WORDS 384U
#define WAIT_QUE_WRITER_RUNNING 0U
#define WAIT_QUE_WRITER_DONE 1U
#define WAIT_QUE_WRITER_FAILED 2U
#define WAIT_MASK_TEST_CYCLES 8U
#define WAIT_MASK_TEST_PERIOD_TICKS 5U
#define WAIT_MASK_TEST_GUARD_TICKS 100U
#define WAIT_MASK_TEST_TIMEOUT_TICKS 20U
#define WAIT_MASK_TEST_STACK_WORDS 384U
#define WAIT_MASK_MODE_BYTE_ANY 0U
#define WAIT_MASK_MODE_BYTE_NONE 1U
#define WAIT_MASK_MODE_WORD_ANY 2U
#define WAIT_MASK_MODE_WORD_NONE 3U
#define WAIT_MASK_MODE_DWORD_ANY 4U
#define WAIT_MASK_MODE_DWORD_NONE 5U
#define WAIT_MASK_BYTE_MASK ((BYTE)0xA4U)
#define WAIT_MASK_WORD_MASK ((WORD)0x8421U)
#define WAIT_MASK_DWORD_MASK ((DWORD)0x84210080UL)

static bool RunTimeWaitTest(void);
static bool RunTimeoutResultTest(void);
static bool RunFlagWaitFunctionalTest(void);
static bool RunSingleFlagWaitFunctionalTest(DWORD Mode);
static void FlagWaiterTask(DWORD FlagPar, DWORD CountPar, DWORD DonePar, DWORD ModePar);
static bool RunBitWaitFunctionalTest(void);
static bool RunSingleBitWaitFunctionalTest(DWORD Mode);
static void BitWaiterTask(DWORD ObjectPar, DWORD CountPar, DWORD DonePar, DWORD ModePar);
static bool RunSemaphoreWaitFunctionalTest(void);
static bool RunSingleSemaphoreWaitFunctionalTest(DWORD Mode);
static bool RunSemaphoreWaitTimeoutTest(void);
static void SemaphoreWaiterTask(DWORD SemPar, DWORD CountPar, DWORD DonePar, DWORD ModePar);
static bool RunQueueWaitFunctionalTest(void);
static void QueueWriterTask(DWORD QuePar, DWORD CountPar, DWORD StatusPar, DWORD UnusedPar);
static bool RunMaskedBitWaitFunctionalTest(void);
static bool RunSingleMaskedBitWaitFunctionalTest(DWORD Mode);
static bool RunMaskedBitTimeoutTest(void);
static void MaskedBitWaiterTask(DWORD ObjectPar, DWORD CountPar, DWORD DonePar, DWORD ModePar);

static volatile Flag WaitTestFlag;

void RTK_RunWaitTests(void) {
    RTK_TestLog("GROUP WAIT START");

    if(RunTimeWaitTest()) {
        RTK_TestPass("WAIT-001");
    } else {
        RTK_TestFail("WAIT-001", "WaitForTime returned before requested delay elapsed");
    }

    if(RunTimeoutResultTest()) {
        RTK_TestPass("WAIT-002");
    } else {
        RTK_TestFail("WAIT-002", "wait timeout result did not match condition state");
    }

    if(RunFlagWaitFunctionalTest()) {
        RTK_TestPass("WAIT-003");
    } else {
        RTK_TestFail("WAIT-003", "flag wait functional sequence mismatch");
    }

    if(RunBitWaitFunctionalTest()) {
        RTK_TestPass("WAIT-004");
    } else {
        RTK_TestFail("WAIT-004", "bit wait functional sequence mismatch");
    }

    if(RunSemaphoreWaitFunctionalTest()) {
        RTK_TestPass("WAIT-005");
    } else {
        RTK_TestFail("WAIT-005", "semaphore wait functional sequence mismatch");
    }

    if(RunQueueWaitFunctionalTest()) {
        RTK_TestPass("WAIT-006");
    } else {
        RTK_TestFail("WAIT-006", "queue wait functional sequence mismatch");
    }

    if(RunMaskedBitWaitFunctionalTest()) {
        RTK_TestPass("WAIT-007");
    } else {
        RTK_TestFail("WAIT-007", "masked bit wait functional sequence mismatch");
    }
}

static bool RunTimeWaitTest(void) {
    DWORD Start=RTK_TestTimestamp();
    WaitForTime(WAIT_TIME_TEST_TICKS);
    DWORD Elapsed=RTK_TestTimestamp() - Start;

    if(Elapsed<WAIT_TIME_TEST_TICKS) {
        RTK_TestPrintf("WaitForTime elapsed ticks too low: %lu/%u",
                       (unsigned long)Elapsed,
                       WAIT_TIME_TEST_TICKS);
        return false;
    }

    return true;
}

/*
					RunTimeoutResultTest

		Verifica che una wait con time out ritorni true se la condizione è già soddisfatta e false se la condizione resta falsa
	fino alla scadenza del time out. Controlla anche che il ritorno per time out non avvenga prima del tempo richiesto.
*/
static bool RunTimeoutResultTest(void) {
    WaitTestFlag=true;
    bool SatisfiedResult=CheckAndWaitForFlagTO((Flag *)&WaitTestFlag, WAIT_TIMEOUT_TEST_TICKS);
    if(!SatisfiedResult) {
        RTK_TestPrintf("Wait flag satisfied case returned false");
        return false;
    }

    WaitTestFlag=false;
    DWORD TimeoutStart=RTK_TestTimestamp();
    bool TimeoutResult=CheckAndWaitForFlagTO((Flag *)&WaitTestFlag, WAIT_TIMEOUT_TEST_TICKS);
    DWORD TimeoutElapsed=RTK_TestTimestamp() - TimeoutStart;

    if(TimeoutResult || TimeoutElapsed<WAIT_TIMEOUT_TEST_TICKS) {
        RTK_TestPrintf("Wait flag timeout mismatch: result=%u elapsed=%lu/%u",
                       TimeoutResult ? 1U : 0U,
                       (unsigned long)TimeoutElapsed,
                       WAIT_TIMEOUT_TEST_TICKS);
        return false;
    }

    return true;
}

/*
					FlagWaiterTask

	da controllare
		Attende ciclicamente il valore richiesto del flag passato dalla task principale, ripristina il valore opposto e incrementa
		il contatore passato come parametro. Serve anche a provare la creazione di task con parametri multipli.
*/
static void FlagWaiterTask(DWORD FlagPar, DWORD CountPar, DWORD DonePar, DWORD ModePar) {
    volatile Flag *TestFlag=(volatile Flag *)FlagPar;
    volatile DWORD *WaitCount=(volatile DWORD *)CountPar;
    volatile Flag *WaiterDone=(volatile Flag *)DonePar;

    for(DWORD Cycle=0; Cycle<WAIT_FLAG_TEST_CYCLES; Cycle++) {
        if(ModePar==WAIT_FLAG_MODE_SET) {
            WaitForFlag((Flag *)TestFlag);
            *TestFlag=false;
        } else {
            WaitForNotFlag((Flag *)TestFlag);
            *TestFlag=true;
        }
        (*WaitCount)++;
    }

    *WaiterDone=true;
}

/*
					RunSingleFlagWaitFunctionalTest

	da controllare
		Esegue una sequenza funzionale su WaitForFlag oppure WaitForNotFlag. La task principale genera gli eventi, controlla che
		il contatore della task in wait sia congruente ad ogni ciclo e attende la conclusione della task secondaria.
*/
static bool RunSingleFlagWaitFunctionalTest(DWORD Mode) {
    volatile Flag TestFlag=(Mode==WAIT_FLAG_MODE_SET) ? false : true;
    volatile DWORD WaitCount=0;
    volatile Flag WaiterDone=false;

    T_TaskDescriptor *WaiterTask=CreateNamedMultiParsTask(FlagWaiterTask, RTK_Pack("WFLAG      "),
                                                         WAIT_FLAG_TEST_STACK_WORDS, TaskPriorityHi,
                                                         (DWORD)&TestFlag, (DWORD)&WaitCount,
                                                         (DWORD)&WaiterDone, Mode);
    if(WaiterTask==NULL) {
        RTK_TestPrintf("Flag waiter task creation failed");
        return false;
    }

    for(DWORD Cycle=0; Cycle<WAIT_FLAG_TEST_CYCLES; Cycle++) {
        WaitForTime(WAIT_FLAG_TEST_PERIOD_TICKS);
        if(WaitCount!=Cycle) {
            RTK_TestPrintf("Flag wait count changed before event: %lu/%lu",
                           (unsigned long)WaitCount, (unsigned long)Cycle);
            if(!WaiterDone) KillTask(WaiterTask);
            return false;
        }

        TestFlag=(Mode==WAIT_FLAG_MODE_SET) ? true : false;
        DWORD Start=RTK_TestTimestamp();
        while(WaitCount!=(Cycle+1U)) {
            if((RTK_TestTimestamp()-Start)>WAIT_FLAG_TEST_GUARD_TICKS) {
                RTK_TestPrintf("Flag wait guard expired: count=%lu cycle=%lu",
                               (unsigned long)WaitCount, (unsigned long)Cycle);
                if(!WaiterDone) KillTask(WaiterTask);
                return false;
            }
            WaitForTime(1U);
        }
    }

    return WaiterDone && WaitCount==WAIT_FLAG_TEST_CYCLES;
}

/*
					RunFlagWaitFunctionalTest

	da controllare
		Verifica il funzionamento base di WaitForFlag e WaitForNotFlag con una task secondaria che riceve flag e contatore tramite
		parametri multipli, senza usare variabili globali dedicate al test.
*/
static bool RunFlagWaitFunctionalTest(void) {
    return RunSingleFlagWaitFunctionalTest(WAIT_FLAG_MODE_SET) &&
           RunSingleFlagWaitFunctionalTest(WAIT_FLAG_MODE_RESET);
}

/*
					BitWaiterTask

	da controllare
		Attende ciclicamente il bit richiesto sull'oggetto byte, word o dword passato dalla task principale, ripristina il valore
		opposto e incrementa il contatore ricevuto come parametro.
*/
static void BitWaiterTask(DWORD ObjectPar, DWORD CountPar, DWORD DonePar, DWORD ModePar) {
    volatile DWORD *WaitCount=(volatile DWORD *)CountPar;
    volatile Flag *WaiterDone=(volatile Flag *)DonePar;

    for(DWORD Cycle=0; Cycle<WAIT_BIT_TEST_CYCLES; Cycle++) {
        switch(ModePar) {
            case WAIT_BIT_MODE_BYTE_SET:
                WaitForBit((volatile BYTE *)ObjectPar, WAIT_BIT_TEST_BYTE_BIT);
                *((volatile BYTE *)ObjectPar)&=~(BYTE)(1U<<WAIT_BIT_TEST_BYTE_BIT);
                break;
            case WAIT_BIT_MODE_BYTE_RESET:
                WaitForNotBit((volatile BYTE *)ObjectPar, WAIT_BIT_TEST_BYTE_BIT);
                *((volatile BYTE *)ObjectPar)|=(BYTE)(1U<<WAIT_BIT_TEST_BYTE_BIT);
                break;
            case WAIT_BIT_MODE_WORD_SET:
                WaitForWordBit((volatile WORD *)ObjectPar, WAIT_BIT_TEST_WORD_BIT);
                *((volatile WORD *)ObjectPar)&=~(WORD)(1U<<WAIT_BIT_TEST_WORD_BIT);
                break;
            case WAIT_BIT_MODE_WORD_RESET:
                WaitForWordNotBit((volatile WORD *)ObjectPar, WAIT_BIT_TEST_WORD_BIT);
                *((volatile WORD *)ObjectPar)|=(WORD)(1U<<WAIT_BIT_TEST_WORD_BIT);
                break;
            case WAIT_BIT_MODE_DWORD_SET:
                WaitForDWordBit((volatile DWORD *)ObjectPar, WAIT_BIT_TEST_DWORD_BIT);
                *((volatile DWORD *)ObjectPar)&=~(DWORD)(1UL<<WAIT_BIT_TEST_DWORD_BIT);
                break;
            default:
                WaitForDWordNotBit((volatile DWORD *)ObjectPar, WAIT_BIT_TEST_DWORD_BIT);
                *((volatile DWORD *)ObjectPar)|=(DWORD)(1UL<<WAIT_BIT_TEST_DWORD_BIT);
                break;
        }
        (*WaitCount)++;
    }

    *WaiterDone=true;
}

/*
					RunSingleBitWaitFunctionalTest

	da controllare
		Esegue una sequenza funzionale su una wait di bit. La task principale tiene l'oggetto locale, genera gli eventi e controlla
		che la task secondaria si risvegli una volta per ogni modifica prodotta.
*/
static bool RunSingleBitWaitFunctionalTest(DWORD Mode) {
    volatile BYTE TestByte=0;
    volatile WORD TestWord=0;
    volatile DWORD TestDWord=0;
    volatile DWORD WaitCount=0;
    volatile Flag WaiterDone=false;
    volatile void *Object=&TestByte;

    if(Mode==WAIT_BIT_MODE_BYTE_RESET) {
        TestByte=(BYTE)(1U<<WAIT_BIT_TEST_BYTE_BIT);
    } else if(Mode==WAIT_BIT_MODE_WORD_SET || Mode==WAIT_BIT_MODE_WORD_RESET) {
        Object=&TestWord;
        if(Mode==WAIT_BIT_MODE_WORD_RESET) TestWord=(WORD)(1U<<WAIT_BIT_TEST_WORD_BIT);
    } else if(Mode==WAIT_BIT_MODE_DWORD_SET || Mode==WAIT_BIT_MODE_DWORD_RESET) {
        Object=&TestDWord;
        if(Mode==WAIT_BIT_MODE_DWORD_RESET) TestDWord=(DWORD)(1UL<<WAIT_BIT_TEST_DWORD_BIT);
    }

    T_TaskDescriptor *WaiterTask=CreateNamedMultiParsTask(BitWaiterTask, RTK_Pack("WBIT       "),
                                                         WAIT_BIT_TEST_STACK_WORDS, TaskPriorityHi,
                                                         (DWORD)Object, (DWORD)&WaitCount,
                                                         (DWORD)&WaiterDone, Mode);
    if(WaiterTask==NULL) {
        RTK_TestPrintf("Bit waiter task creation failed");
        return false;
    }

    for(DWORD Cycle=0; Cycle<WAIT_BIT_TEST_CYCLES; Cycle++) {
        WaitForTime(WAIT_BIT_TEST_PERIOD_TICKS);
        if(WaitCount!=Cycle) {
            RTK_TestPrintf("Bit wait count changed before event: mode=%lu count=%lu cycle=%lu",
                           (unsigned long)Mode, (unsigned long)WaitCount, (unsigned long)Cycle);
            if(!WaiterDone) KillTask(WaiterTask);
            return false;
        }

        if(Mode==WAIT_BIT_MODE_BYTE_SET) TestByte|=(BYTE)(1U<<WAIT_BIT_TEST_BYTE_BIT);
        else if(Mode==WAIT_BIT_MODE_BYTE_RESET) TestByte&=~(BYTE)(1U<<WAIT_BIT_TEST_BYTE_BIT);
        else if(Mode==WAIT_BIT_MODE_WORD_SET) TestWord|=(WORD)(1U<<WAIT_BIT_TEST_WORD_BIT);
        else if(Mode==WAIT_BIT_MODE_WORD_RESET) TestWord&=~(WORD)(1U<<WAIT_BIT_TEST_WORD_BIT);
        else if(Mode==WAIT_BIT_MODE_DWORD_SET) TestDWord|=(DWORD)(1UL<<WAIT_BIT_TEST_DWORD_BIT);
        else TestDWord&=~(DWORD)(1UL<<WAIT_BIT_TEST_DWORD_BIT);

        DWORD Start=RTK_TestTimestamp();
        while(WaitCount!=(Cycle+1U)) {
            if((RTK_TestTimestamp()-Start)>WAIT_BIT_TEST_GUARD_TICKS) {
                RTK_TestPrintf("Bit wait guard expired: mode=%lu count=%lu cycle=%lu",
                               (unsigned long)Mode, (unsigned long)WaitCount, (unsigned long)Cycle);
                if(!WaiterDone) KillTask(WaiterTask);
                return false;
            }
            WaitForTime(1U);
        }
    }

    return WaiterDone && WaitCount==WAIT_BIT_TEST_CYCLES;
}

/*
					RunBitWaitFunctionalTest

	da controllare
		Verifica le wait su bit abilitate per byte, word e dword, sia in attesa del bit a uno sia in attesa del bit a zero.
*/
static bool RunBitWaitFunctionalTest(void) {
    return RunSingleBitWaitFunctionalTest(WAIT_BIT_MODE_BYTE_SET) &&
           RunSingleBitWaitFunctionalTest(WAIT_BIT_MODE_BYTE_RESET) &&
           RunSingleBitWaitFunctionalTest(WAIT_BIT_MODE_WORD_SET) &&
           RunSingleBitWaitFunctionalTest(WAIT_BIT_MODE_WORD_RESET) &&
           RunSingleBitWaitFunctionalTest(WAIT_BIT_MODE_DWORD_SET) &&
           RunSingleBitWaitFunctionalTest(WAIT_BIT_MODE_DWORD_RESET);
}

/*
					SemaphoreWaiterTask

	da controllare
		Attende ciclicamente un semaforo binario o counting passato dalla task principale e incrementa il contatore ricevuto come
		parametro ogni volta che la wait viene soddisfatta.
*/
static void SemaphoreWaiterTask(DWORD SemPar, DWORD CountPar, DWORD DonePar, DWORD ModePar) {
    volatile DWORD *WaitCount=(volatile DWORD *)CountPar;
    volatile Flag *WaiterDone=(volatile Flag *)DonePar;

    for(DWORD Cycle=0; Cycle<WAIT_SEM_TEST_CYCLES; Cycle++) {
        if(ModePar==WAIT_SEM_MODE_BINARY) {
            WaitForSem((Semaphore *)SemPar);
        } else {
            WaitForCountingSem((T_CountingSem *)SemPar);
        }
        (*WaitCount)++;
    }

    *WaiterDone=true;
}

/*
					RunSingleSemaphoreWaitFunctionalTest

	da controllare
		Verifica che WaitForSem oppure WaitForCountingSem risvegli la task secondaria una sola volta per ogni evento prodotto
		dalla task principale, usando oggetto e contatori locali passati come parametri.
*/
static bool RunSingleSemaphoreWaitFunctionalTest(DWORD Mode) {
    Semaphore BinarySem=SEM_LOCKED;
    T_CountingSem CountingSem=0U;
    volatile DWORD WaitCount=0;
    volatile Flag WaiterDone=false;
    DWORD SemPar=(Mode==WAIT_SEM_MODE_BINARY) ? (DWORD)&BinarySem : (DWORD)&CountingSem;

    T_TaskDescriptor *WaiterTask=CreateNamedMultiParsTask(SemaphoreWaiterTask, RTK_Pack("WSEM       "),
                                                         WAIT_SEM_TEST_STACK_WORDS, TaskPriorityHi,
                                                         SemPar, (DWORD)&WaitCount,
                                                         (DWORD)&WaiterDone, Mode);
    if(WaiterTask==NULL) {
        RTK_TestPrintf("Semaphore wait task creation failed");
        return false;
    }

    for(DWORD Cycle=0; Cycle<WAIT_SEM_TEST_CYCLES; Cycle++) {
        WaitForTime(WAIT_SEM_TEST_PERIOD_TICKS);
        if(WaitCount!=Cycle) {
            RTK_TestPrintf("Semaphore wait count changed before event: mode=%lu count=%lu cycle=%lu",
                           (unsigned long)Mode, (unsigned long)WaitCount, (unsigned long)Cycle);
            if(!WaiterDone) KillTask(WaiterTask);
            return false;
        }

        if(Mode==WAIT_SEM_MODE_BINARY) Release(&BinarySem);
        else PutCountingSem(&CountingSem);

        DWORD Start=RTK_TestTimestamp();
        while(WaitCount!=(Cycle+1U)) {
            if((RTK_TestTimestamp()-Start)>WAIT_SEM_TEST_GUARD_TICKS) {
                RTK_TestPrintf("Semaphore wait guard expired: mode=%lu count=%lu cycle=%lu",
                               (unsigned long)Mode, (unsigned long)WaitCount, (unsigned long)Cycle);
                if(!WaiterDone) KillTask(WaiterTask);
                return false;
            }
            WaitForTime(1U);
        }
    }

    return WaiterDone && WaitCount==WAIT_SEM_TEST_CYCLES;
}

/*
					RunSemaphoreWaitTimeoutTest

	da controllare
		Verifica i percorsi con time out delle wait su semaforo, controllando sia il caso già soddisfatto sia il caso in cui la
		condizione resta falsa fino alla scadenza del time out.
*/
static bool RunSemaphoreWaitTimeoutTest(void) {
    Semaphore BinarySem=SEM_FREE;
    T_CountingSem CountingSem=1U;

    if(!CheckAndWaitForSemTO(&BinarySem, WAIT_SEM_TEST_TIMEOUT_TICKS)) {
        RTK_TestPrintf("Binary semaphore immediate timeout wait returned false");
        return false;
    }

    DWORD Start=RTK_TestTimestamp();
    bool BinaryTimeoutResult=CheckAndWaitForSemTO(&BinarySem, WAIT_SEM_TEST_TIMEOUT_TICKS);
    DWORD Elapsed=RTK_TestTimestamp()-Start;
    if(BinaryTimeoutResult || Elapsed<WAIT_SEM_TEST_TIMEOUT_TICKS) {
        RTK_TestPrintf("Binary semaphore timeout mismatch: result=%u elapsed=%lu/%u",
                       BinaryTimeoutResult ? 1U : 0U, (unsigned long)Elapsed, WAIT_SEM_TEST_TIMEOUT_TICKS);
        Release(&BinarySem);
        return false;
    }
    Release(&BinarySem);

    if(!CheckAndWaitForCountingSemTO(&CountingSem, WAIT_SEM_TEST_TIMEOUT_TICKS)) {
        RTK_TestPrintf("Counting semaphore immediate timeout wait returned false");
        return false;
    }

    Start=RTK_TestTimestamp();
    bool CountingTimeoutResult=CheckAndWaitForCountingSemTO(&CountingSem, WAIT_SEM_TEST_TIMEOUT_TICKS);
    Elapsed=RTK_TestTimestamp()-Start;
    if(CountingTimeoutResult || Elapsed<WAIT_SEM_TEST_TIMEOUT_TICKS) {
        RTK_TestPrintf("Counting semaphore timeout mismatch: result=%u elapsed=%lu/%u",
                       CountingTimeoutResult ? 1U : 0U, (unsigned long)Elapsed, WAIT_SEM_TEST_TIMEOUT_TICKS);
        return false;
    }

    return true;
}

/*
					RunSemaphoreWaitFunctionalTest

	da controllare
		Verifica le wait abilitate sui semafori binari e counting, includendo le primitive con time out.
*/
static bool RunSemaphoreWaitFunctionalTest(void) {
    return RunSingleSemaphoreWaitFunctionalTest(WAIT_SEM_MODE_BINARY) &&
           RunSingleSemaphoreWaitFunctionalTest(WAIT_SEM_MODE_COUNTING) &&
           RunSemaphoreWaitTimeoutTest();
}

/*
					QueueWriterTask

	da controllare
		Scrive una sequenza nota nella coda byte passata dalla task principale, attendendo spazio prima di ogni put. Il contatore
		e lo stato finale sono passati come parametri per evitare variabili globali dedicate al test.
*/
static void QueueWriterTask(DWORD QuePar, DWORD CountPar, DWORD StatusPar, DWORD UnusedPar) {
    TByteQue *Que=(TByteQue *)QuePar;
    volatile DWORD *PutCount=(volatile DWORD *)CountPar;
    volatile DWORD *WriterStatus=(volatile DWORD *)StatusPar;
    (void)UnusedPar;

    for(DWORD Ch=0; Ch<WAIT_QUE_TEST_ITEMS; Ch++) {
        WaitForBynaryLenQuePut(&Que->BinaryLenQueHeader);
        if(!QuePut(Que, (BYTE)Ch)) {
            *WriterStatus=WAIT_QUE_WRITER_FAILED;
            return;
        }
        (*PutCount)++;
    }

    *WriterStatus=WAIT_QUE_WRITER_DONE;
}

/*
					RunQueueWaitFunctionalTest

	da controllare
		Verifica le wait abilitate su coda byte: attesa di dati disponibili, attesa di spazio per put e attesa di coda vuota con
		time out. La coda richiesta ha 7 elementi utili, valore che deve restare esatto per la coda binaria.
*/
static bool RunQueueWaitFunctionalTest(void) {
    TByteQue *Que=NewQue(WAIT_QUE_TEST_SIZE);
    volatile DWORD PutCount=0;
    volatile DWORD WriterStatus=WAIT_QUE_WRITER_RUNNING;

    if(Que==NULL) {
        RTK_TestPrintf("Queue creation failed");
        return false;
    }
    if(BinaryLenQueSize(&Que->BinaryLenQueHeader)!=WAIT_QUE_TEST_SIZE) {
        RTK_TestPrintf("Queue size mismatch: %u/%u", BinaryLenQueSize(&Que->BinaryLenQueHeader), WAIT_QUE_TEST_SIZE);
        free(Que);
        return false;
    }

    T_TaskDescriptor *WriterTask=CreateNamedMultiParsTask(QueueWriterTask, RTK_Pack("WQUE       "),
                                                         WAIT_QUE_TEST_STACK_WORDS, TaskPriorityHi,
                                                         (DWORD)Que, (DWORD)&PutCount,
                                                         (DWORD)&WriterStatus, 0U);
    if(WriterTask==NULL) {
        RTK_TestPrintf("Queue writer task creation failed");
        free(Que);
        return false;
    }

    for(DWORD Expected=0; Expected<WAIT_QUE_TEST_ITEMS; Expected++) {
        if(!CheckAndWaitForQueGetTO(&Que->BinaryLenQueHeader.QueHeader, WAIT_QUE_TEST_TIMEOUT_TICKS)) {
            RTK_TestPrintf("Queue get timeout: expected=%lu put=%lu",
                           (unsigned long)Expected, (unsigned long)PutCount);
            if(WriterStatus==WAIT_QUE_WRITER_RUNNING) KillTask(WriterTask);
            free(Que);
            return false;
        }

        WORD Ch=QueGet(Que);
        if((Ch&0xFF00U)==0U || (BYTE)Ch!=(BYTE)Expected) {
            RTK_TestPrintf("Queue data mismatch: got=%u expected=%lu",
                           (unsigned)(BYTE)Ch, (unsigned long)Expected);
            if(WriterStatus==WAIT_QUE_WRITER_RUNNING) KillTask(WriterTask);
            free(Que);
            return false;
        }
    }

    DWORD Start=RTK_TestTimestamp();
    while(WriterStatus==WAIT_QUE_WRITER_RUNNING) {
        if((RTK_TestTimestamp()-Start)>WAIT_QUE_TEST_GUARD_TICKS) {
            RTK_TestPrintf("Queue writer completion guard expired: put=%lu", (unsigned long)PutCount);
            KillTask(WriterTask);
            free(Que);
            return false;
        }
        WaitForTime(1U);
    }

    if(WriterStatus!=WAIT_QUE_WRITER_DONE || PutCount!=WAIT_QUE_TEST_ITEMS) {
        RTK_TestPrintf("Queue writer status mismatch: status=%lu put=%lu",
                       (unsigned long)WriterStatus, (unsigned long)PutCount);
        free(Que);
        return false;
    }

    if(!CheckAndWaitForQueEmptyTO(&Que->BinaryLenQueHeader.QueHeader, WAIT_QUE_TEST_TIMEOUT_TICKS)) {
        RTK_TestPrintf("Queue empty wait failed on empty queue");
        free(Que);
        return false;
    }
    if(!QuePut(Que, 0xA5U)) {
        RTK_TestPrintf("Queue single put failed");
        free(Que);
        return false;
    }

    Start=RTK_TestTimestamp();
    bool EmptyResult=CheckAndWaitForQueEmptyTO(&Que->BinaryLenQueHeader.QueHeader, WAIT_QUE_TEST_TIMEOUT_TICKS);
    DWORD Elapsed=RTK_TestTimestamp()-Start;
    if(EmptyResult || Elapsed<WAIT_QUE_TEST_TIMEOUT_TICKS) {
        RTK_TestPrintf("Queue empty timeout mismatch: result=%u elapsed=%lu/%u",
                       EmptyResult ? 1U : 0U, (unsigned long)Elapsed, WAIT_QUE_TEST_TIMEOUT_TICKS);
        free(Que);
        return false;
    }

    WORD Ch=QueGet(Que);
    if((Ch&0xFF00U)==0U || (BYTE)Ch!=0xA5U) {
        RTK_TestPrintf("Queue single get mismatch: %u", (unsigned)(BYTE)Ch);
        free(Que);
        return false;
    }
    if(!CheckAndWaitForQueEmptyTO(&Que->BinaryLenQueHeader.QueHeader, WAIT_QUE_TEST_TIMEOUT_TICKS)) {
        RTK_TestPrintf("Queue empty wait failed after final get");
        free(Que);
        return false;
    }

    free(Que);
    return true;
}

/*
					MaskedBitWaiterTask

	da controllare
		Attende ciclicamente una condizione su maschera byte, word o dword e ripristina lo stato opposto dopo ogni risveglio.
*/
static void MaskedBitWaiterTask(DWORD ObjectPar, DWORD CountPar, DWORD DonePar, DWORD ModePar) {
    volatile DWORD *WaitCount=(volatile DWORD *)CountPar;
    volatile Flag *WaiterDone=(volatile Flag *)DonePar;

    for(DWORD Cycle=0; Cycle<WAIT_MASK_TEST_CYCLES; Cycle++) {
        switch(ModePar) {
            case WAIT_MASK_MODE_BYTE_ANY:
                WaitForAlmenoUnBit((volatile BYTE *)ObjectPar, WAIT_MASK_BYTE_MASK);
                *((volatile BYTE *)ObjectPar)&=(BYTE)~WAIT_MASK_BYTE_MASK;
                break;
            case WAIT_MASK_MODE_BYTE_NONE:
                WaitForNessunBit((volatile BYTE *)ObjectPar, WAIT_MASK_BYTE_MASK);
                *((volatile BYTE *)ObjectPar)|=WAIT_MASK_BYTE_MASK;
                break;
            case WAIT_MASK_MODE_WORD_ANY:
                WaitForAlmenoUnWordBit((volatile WORD *)ObjectPar, WAIT_MASK_WORD_MASK);
                *((volatile WORD *)ObjectPar)&=(WORD)~WAIT_MASK_WORD_MASK;
                break;
            case WAIT_MASK_MODE_WORD_NONE:
                WaitForNessunWordBit((volatile WORD *)ObjectPar, WAIT_MASK_WORD_MASK);
                *((volatile WORD *)ObjectPar)|=WAIT_MASK_WORD_MASK;
                break;
            case WAIT_MASK_MODE_DWORD_ANY:
                WaitForAlmenoUnDWordBit((volatile DWORD *)ObjectPar, WAIT_MASK_DWORD_MASK);
                *((volatile DWORD *)ObjectPar)&=(DWORD)~WAIT_MASK_DWORD_MASK;
                break;
            default:
                WaitForNessunDWordBit((volatile DWORD *)ObjectPar, WAIT_MASK_DWORD_MASK);
                *((volatile DWORD *)ObjectPar)|=WAIT_MASK_DWORD_MASK;
                break;
        }
        (*WaitCount)++;
    }

    *WaiterDone=true;
}

/*
					RunSingleMaskedBitWaitFunctionalTest

	da controllare
		Esegue una sequenza funzionale su una wait a maschera. La task principale genera gli eventi e controlla che la task in wait
		si risvegli esattamente una volta per ogni variazione prodotta.
*/
static bool RunSingleMaskedBitWaitFunctionalTest(DWORD Mode) {
    volatile BYTE TestByte=0;
    volatile WORD TestWord=0;
    volatile DWORD TestDWord=0;
    volatile DWORD WaitCount=0;
    volatile Flag WaiterDone=false;
    volatile void *Object=&TestByte;

    if(Mode==WAIT_MASK_MODE_BYTE_NONE) TestByte=WAIT_MASK_BYTE_MASK;
    else if(Mode==WAIT_MASK_MODE_WORD_ANY || Mode==WAIT_MASK_MODE_WORD_NONE) {
        Object=&TestWord;
        if(Mode==WAIT_MASK_MODE_WORD_NONE) TestWord=WAIT_MASK_WORD_MASK;
    } else if(Mode==WAIT_MASK_MODE_DWORD_ANY || Mode==WAIT_MASK_MODE_DWORD_NONE) {
        Object=&TestDWord;
        if(Mode==WAIT_MASK_MODE_DWORD_NONE) TestDWord=WAIT_MASK_DWORD_MASK;
    }

    T_TaskDescriptor *WaiterTask=CreateNamedMultiParsTask(MaskedBitWaiterTask, RTK_Pack("WMASK      "),
                                                         WAIT_MASK_TEST_STACK_WORDS, TaskPriorityHi,
                                                         (DWORD)Object, (DWORD)&WaitCount,
                                                         (DWORD)&WaiterDone, Mode);
    if(WaiterTask==NULL) {
        RTK_TestPrintf("Masked bit waiter task creation failed");
        return false;
    }

    for(DWORD Cycle=0; Cycle<WAIT_MASK_TEST_CYCLES; Cycle++) {
        WaitForTime(WAIT_MASK_TEST_PERIOD_TICKS);
        if(WaitCount!=Cycle) {
            RTK_TestPrintf("Masked bit count changed before event: mode=%lu count=%lu cycle=%lu",
                           (unsigned long)Mode, (unsigned long)WaitCount, (unsigned long)Cycle);
            if(!WaiterDone) KillTask(WaiterTask);
            return false;
        }

        if(Mode==WAIT_MASK_MODE_BYTE_ANY) TestByte|=0x20U;
        else if(Mode==WAIT_MASK_MODE_BYTE_NONE) TestByte&=(BYTE)~WAIT_MASK_BYTE_MASK;
        else if(Mode==WAIT_MASK_MODE_WORD_ANY) TestWord|=0x0400U;
        else if(Mode==WAIT_MASK_MODE_WORD_NONE) TestWord&=(WORD)~WAIT_MASK_WORD_MASK;
        else if(Mode==WAIT_MASK_MODE_DWORD_ANY) TestDWord|=0x00200000UL;
        else TestDWord&=(DWORD)~WAIT_MASK_DWORD_MASK;

        DWORD Start=RTK_TestTimestamp();
        while(WaitCount!=(Cycle+1U)) {
            if((RTK_TestTimestamp()-Start)>WAIT_MASK_TEST_GUARD_TICKS) {
                RTK_TestPrintf("Masked bit guard expired: mode=%lu count=%lu cycle=%lu",
                               (unsigned long)Mode, (unsigned long)WaitCount, (unsigned long)Cycle);
                if(!WaiterDone) KillTask(WaiterTask);
                return false;
            }
            WaitForTime(1U);
        }
    }

    return WaiterDone && WaitCount==WAIT_MASK_TEST_CYCLES;
}

/*
					RunMaskedBitTimeoutTest

	da controllare
		Verifica il percorso con time out delle wait a maschera abilitate, sia per condizione già soddisfatta sia per scadenza.
*/
static bool RunMaskedBitTimeoutTest(void) {
    volatile BYTE TestByte=WAIT_MASK_BYTE_MASK;
    volatile WORD TestWord=WAIT_MASK_WORD_MASK;
    volatile DWORD TestDWord=WAIT_MASK_DWORD_MASK;

    if(!CheckAndWaitForAlmenoUnBitTO(&TestByte, WAIT_MASK_BYTE_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS)) return false;
    if(!CheckAndWaitForAlmenoUnWordBitTO(&TestWord, WAIT_MASK_WORD_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS)) return false;
    if(!CheckAndWaitForAlmenoUnDWordBitTO(&TestDWord, WAIT_MASK_DWORD_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS)) return false;

    TestByte=0;
    TestWord=0;
    TestDWord=0;
    if(!CheckAndWaitForNessunBitTO(&TestByte, WAIT_MASK_BYTE_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS)) return false;
    if(!CheckAndWaitForNessunWordBitTO(&TestWord, WAIT_MASK_WORD_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS)) return false;
    if(!CheckAndWaitForNessunDWordBitTO(&TestDWord, WAIT_MASK_DWORD_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS)) return false;

    DWORD Start=RTK_TestTimestamp();
    bool Result=WaitForAlmenoUnBitTO(&TestByte, WAIT_MASK_BYTE_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS);
    if(Result || (RTK_TestTimestamp()-Start)<WAIT_MASK_TEST_TIMEOUT_TICKS) return false;

    Start=RTK_TestTimestamp();
    Result=WaitForAlmenoUnWordBitTO(&TestWord, WAIT_MASK_WORD_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS);
    if(Result || (RTK_TestTimestamp()-Start)<WAIT_MASK_TEST_TIMEOUT_TICKS) return false;

    Start=RTK_TestTimestamp();
    Result=WaitForAlmenoUnDWordBitTO(&TestDWord, WAIT_MASK_DWORD_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS);
    if(Result || (RTK_TestTimestamp()-Start)<WAIT_MASK_TEST_TIMEOUT_TICKS) return false;

    TestByte=WAIT_MASK_BYTE_MASK;
    TestWord=WAIT_MASK_WORD_MASK;
    TestDWord=WAIT_MASK_DWORD_MASK;

    Start=RTK_TestTimestamp();
    Result=WaitForNessunBitTO(&TestByte, WAIT_MASK_BYTE_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS);
    if(Result || (RTK_TestTimestamp()-Start)<WAIT_MASK_TEST_TIMEOUT_TICKS) return false;

    Start=RTK_TestTimestamp();
    Result=WaitForNessunWordBitTO(&TestWord, WAIT_MASK_WORD_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS);
    if(Result || (RTK_TestTimestamp()-Start)<WAIT_MASK_TEST_TIMEOUT_TICKS) return false;

    Start=RTK_TestTimestamp();
    Result=WaitForNessunDWordBitTO(&TestDWord, WAIT_MASK_DWORD_MASK, WAIT_MASK_TEST_TIMEOUT_TICKS);
    if(Result || (RTK_TestTimestamp()-Start)<WAIT_MASK_TEST_TIMEOUT_TICKS) return false;

    return true;
}

/*
					RunMaskedBitWaitFunctionalTest

	da controllare
		Verifica le wait a maschera su byte, word e dword, sia almeno un bit della maschera sia nessun bit della maschera.
*/
static bool RunMaskedBitWaitFunctionalTest(void) {
    return RunSingleMaskedBitWaitFunctionalTest(WAIT_MASK_MODE_BYTE_ANY) &&
           RunSingleMaskedBitWaitFunctionalTest(WAIT_MASK_MODE_BYTE_NONE) &&
           RunSingleMaskedBitWaitFunctionalTest(WAIT_MASK_MODE_WORD_ANY) &&
           RunSingleMaskedBitWaitFunctionalTest(WAIT_MASK_MODE_WORD_NONE) &&
           RunSingleMaskedBitWaitFunctionalTest(WAIT_MASK_MODE_DWORD_ANY) &&
           RunSingleMaskedBitWaitFunctionalTest(WAIT_MASK_MODE_DWORD_NONE) &&
           RunMaskedBitTimeoutTest();
}
