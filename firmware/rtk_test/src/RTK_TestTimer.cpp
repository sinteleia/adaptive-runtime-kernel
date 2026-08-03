#include "RTK_TestTimer.h"
#include "RTK_TestDiag.h"
#include "RTK.h"
#include "TimerTic.h"

#define TIMER_TEST_COUNT       5U
#define TIMER_TEST_WAIT_TICKS  50U

static bool RunOrderedTimerTest(void);
static bool TimerListContainsOnce(T_Timer *Timer);
static bool TimerListIsOrdered(void);

void RTK_RunTimerTests(void) {
    RTK_TestLog("GROUP TIMER START");

    if(RunOrderedTimerTest()) {
        RTK_TestPass("TIMER-001");
    } else {
        RTK_TestFail("TIMER-001", "timer list ordering or expiration mismatch");
    }
}

/*
					RunOrderedTimerTest

	da controllare
		Verifica che SetTimer inserisca i timer in ordine di scadenza, che due timer con la stessa scadenza restino entrambi in
		lista, che DisarmaTimer rimuova un timer non ancora scaduto e che TimerTic li marchi scaduti dopo il tempo atteso.
*/
static bool RunOrderedTimerTest(void) {
    T_Timer Timers[TIMER_TEST_COUNT];

    for(BYTE Index=0; Index<TIMER_TEST_COUNT; Index++) {
        InitTimer(&Timers[Index]);
    }

    SetTimer(&Timers[0], 30U);
    SetTimer(&Timers[1], 10U);
    SetTimer(&Timers[2], 20U);
    SetTimer(&Timers[3], 40U);
    SetTimer(&Timers[4], 20U);

    if(!TimerListIsOrdered()) {
        RTK_TestPrintf("Timer list not ordered after SetTimer\n");
        return false;
    }
    for(BYTE Index=0; Index<TIMER_TEST_COUNT; Index++) {
        if(!TimerListContainsOnce(&Timers[Index])) {
            RTK_TestPrintf("Timer %u missing or duplicated in list\n", Index);
            return false;
        }
    }

    DisarmaTimer(&Timers[3]);
    if(!IsTimerPtrElapsed(&Timers[3]) || TimerListContainsOnce(&Timers[3])) {
        RTK_TestPrintf("Timer disarm did not remove timer\n");
        return false;
    }
    if(!TimerListIsOrdered()) {
        RTK_TestPrintf("Timer list not ordered after DisarmaTimer\n");
        return false;
    }

    WaitForTime(TIMER_TEST_WAIT_TICKS);

    for(BYTE Index=0; Index<TIMER_TEST_COUNT; Index++) {
        if(!IsTimerPtrElapsed(&Timers[Index])) {
            RTK_TestPrintf("Timer %u did not expire\n", Index);
            return false;
        }
    }

    return true;
}

/*
					TimerListContainsOnce

	da controllare
		Scandisce la lista dei timer attivi e verifica che il timer indicato sia presente una sola volta.
*/
static bool TimerListContainsOnce(T_Timer *Timer) {
    BYTE Found=0;
    T_Timer *Cnt=FirstToTic;

    while(Cnt!=NULL) {
        if(Cnt==Timer) {
            Found++;
        }
        Cnt=Cnt->Next;
    }

    return Found==1U;
}

/*
					TimerListIsOrdered

	da controllare
		Verifica che la lista dei timer attivi sia ordinata per differenza di scadenza rispetto al valore corrente di TimerCtr.
*/
static bool TimerListIsOrdered(void) {
    T_Timer *Cnt=FirstToTic;
    DWORD Previous=0U;

    while(Cnt!=NULL) {
        DWORD Remaining=Cnt->Time - TimerCtr;
        if(Remaining<Previous) {
            return false;
        }
        Previous=Remaining;
        Cnt=Cnt->Next;
    }

    return true;
}
