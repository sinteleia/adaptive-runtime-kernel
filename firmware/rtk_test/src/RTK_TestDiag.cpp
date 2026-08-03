#include "RTK_TestDiag.h"
#include "RTK_TestBoard.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static unsigned PassedTests;
static unsigned FailedTests;
static bool CompactProgress;
static bool CompactLineOpen;

#define RTK_TEST_ID_LEN 10
#define RTK_TEST_DESC_LEN 31

typedef struct RTK_TestResult {
    char Id[RTK_TEST_ID_LEN];
    char Description[RTK_TEST_DESC_LEN];
    unsigned Passed;
    unsigned Failed;
} RTK_TestResult;

static RTK_TestResult TestResults[] = {
    {"SCHED-001", "Priority order", 0, 0},
    {"SCHED-002", "Same prio coverage", 0, 0},
    {"SCHED-003", "Idle task runs", 0, 0},
    {"SCHED-004", "Scheduler returns", 0, 0},
    {"SCHED-005", "High prio preempts", 0, 0},
    {"SCHED-006", "Task termination", 0, 0},
    {"SCHED-007", "PendSV after unlock", 0, 0},
    {"SCHED-008", "PendSV re-pending", 0, 0},
    {"SCHED-009", "Restart with/without init", 0, 0},
    {"SCHED-010", "Pre-init start rejected", 0, 0},
    {"SCHED-011", "Terminate unlocks", 0, 0},
    {"SCHED-012", "Med/low ratio", 0, 0},
    {"WAIT-001", "Time wait", 0, 0},
    {"WAIT-002", "Wait timeout result", 0, 0},
    {"WAIT-003", "Flag waits", 0, 0},
    {"WAIT-004", "Bit waits", 0, 0},
    {"WAIT-005", "Semaphore waits", 0, 0},
    {"WAIT-006", "Queue waits", 0, 0},
    {"WAIT-007", "Masked bit waits", 0, 0},
    {"TIMER-001", "Ordered timers", 0, 0},
    {"SEM-001", "Binary semaphore", 0, 0},
    {"SEM-002", "Counting semaphore", 0, 0},
    {"MM-001", "Dynamic allocation", 0, 0},
    {"MM-002", "Heap protection", 0, 0},
    {"DIAG-001", "Task diagnostics", 0, 0},
    {"DIAG-002", "Idle diagnostics", 0, 0},
    {"DIAG-003", "Terminal failure", 0, 0},
    {"CFG-001", "Valid configuration", 0, 0},
};

static RTK_TestResult *FindTestResult(const char *test_id) {
    for(unsigned i=0; i<(sizeof(TestResults) / sizeof(TestResults[0])); i++) {
        if(strcmp(TestResults[i].Id, test_id)==0) {
            return &TestResults[i];
        }
    }

    return NULL;
}

static void RTK_TestDiagCloseCompactLine(void) {
    if(CompactLineOpen) {
        RTK_TestBoardWrite("\r\n");
        CompactLineOpen=false;
    }
}

void RTK_TestDiagInit(void) {
    PassedTests = 0;
    FailedTests = 0;
    CompactProgress=false;
    CompactLineOpen=false;
    for(unsigned i=0; i<(sizeof(TestResults) / sizeof(TestResults[0])); i++) {
        TestResults[i].Passed=0;
        TestResults[i].Failed=0;
    }
}

void RTK_TestDiagSetCompactProgress(bool Enabled) {
    if(!Enabled) {
        RTK_TestDiagCloseCompactLine();
    }
    CompactProgress=Enabled;
}

void RTK_TestDiagBeginCycle(unsigned Cycle) {
    char Buffer[24];
    RTK_TestDiagSetCompactProgress(false);
    snprintf(Buffer, sizeof(Buffer), "Test %u ", Cycle);
    RTK_TestBoardWrite(Buffer);
    CompactProgress=true;
    CompactLineOpen=true;
}

void RTK_TestDiagEndCycle(void) {
    RTK_TestDiagSetCompactProgress(false);
}

/*
					RTK_TestProgress
	da controllare

		Emette un punto di avanzamento durante una fase lunga del test quando
		il logger compatto e' attivo, senza modificare i contatori PASS/FAIL.
*/
void RTK_TestProgress(void) {
    if(CompactProgress) {
        RTK_TestBoardWrite(".");
        CompactLineOpen=true;
    }
}

void RTK_TestLog(const char *message) {
    if(CompactProgress) {
        return;
    }
    RTK_TestBoardWrite(message);
    RTK_TestBoardWrite("\r\n");
}

void RTK_TestPrintf(const char *format, ...) {
    char buffer[160];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    RTK_TestLog(buffer);
}

void RTK_TestPass(const char *test_id) {
    RTK_TestResult *TestResult=FindTestResult(test_id);
    if(TestResult!=NULL) {
        TestResult->Passed++;
    }
    PassedTests++;
    if(CompactProgress) {
        RTK_TestBoardWrite(".");
        CompactLineOpen=true;
    } else {
        RTK_TestPrintf("TEST %s PASS", test_id);
    }
}

void RTK_TestFail(const char *test_id, const char *reason) {
    RTK_TestResult *TestResult=FindTestResult(test_id);
    if(TestResult!=NULL) {
        TestResult->Failed++;
    }
    FailedTests++;
    RTK_TestDiagSetCompactProgress(false);
    RTK_TestPrintf("TEST %s FAIL %s", test_id, reason);
}

void RTK_TestFatal(const char *reason) {
    RTK_TestDiagSetCompactProgress(false);
    RTK_TestPrintf("FATAL %s", reason);
    for (;;) {
    }
}

void RTK_TestSummary(void) {
    RTK_TestLog("TEST SUMMARY TABLE");
    RTK_TestLog("ID        DESCRIPTION                    PASS FAIL");
    for(unsigned i=0; i<(sizeof(TestResults) / sizeof(TestResults[0])); i++) {
        RTK_TestPrintf("%-9s %-30s %4u %4u",
                       TestResults[i].Id,
                       TestResults[i].Description,
                       TestResults[i].Passed,
                       TestResults[i].Failed);
    }
    RTK_TestPrintf("SUMMARY PASS=%u FAIL=%u", PassedTests, FailedTests);
}
