#include "MainTask.h"
#include "RTK_TestCases.h"
#include "RTK_TestDiag.h"
#include "RTK.h"
#include "RTK_TestBoard.h"
#include "RTK_TestMain.h"
#include <stdio.h>

// #define RTK_BASIC_DEMO_FAULT_TEST 1

#ifndef RTK_BASIC_DEMO_FAULT_TEST
	#define RTK_BASIC_DEMO_FAULT_TEST 0
#endif

#define RTK_BASIC_DEMO_INVALID_FLAG_ADDRESS ((Flag *)0xFFFFFFFFUL)

T_TaskDescriptor *LED_TaskHND;


extern "C" void LED_Task(){
	int i=0;
	while(true){
		i++;
		WaitForTime(100);
		RTK_TestLedToggle(0);
		if(i&1) RTK_TestLedToggle(1);
	}
}

extern unsigned int NumberOfActiveTimers; // This variable normally is not public

extern "C" void MainTask(void) {

	unsigned int InitialTimerNumber=NumberOfActiveTimers;

	LED_TaskHND=CreateNamedTask(LED_Task, RTK_Pack("LED Task    "), 1000, TaskPriorityBackGround);

	WaitForTime(1000);

#if RTK_BASIC_DEMO_FAULT_TEST
	WaitForFlag(RTK_BASIC_DEMO_INVALID_FLAG_ADDRESS);
#endif

    RTK_RunSchedulerTests();
    RTK_RunWaitTests();
    RTK_RunSemaphoreTests();
    RTK_RunTimerTests();
    RTK_RunMemoryTests();

    KillTask(LED_TaskHND);

    T_TimerStatus TimerStatus=CheckTimerStatus();
    switch(TimerStatus){
		case TimerNumberError: RTK_TestFatal("Timer number incongruence!");
		case TimerSequenceError: RTK_TestFatal("Timer sequence incongruence!");
		case TimerExpiredInQue: RTK_TestFatal("Timer expired found in timer que!");
		default: break;
	}
    if(InitialTimerNumber!=NumberOfActiveTimers) RTK_TestFatal("Timer number changed!");

    RTK_TestMainTaskExitCounter++;
}
