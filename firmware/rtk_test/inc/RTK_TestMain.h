#ifndef RTK_TEST_MAIN_H_
	#define RTK_TEST_MAIN_H_

	#include "type.h"

	#define RTK_TEST_MEDIUM_FOR_LOW 5

	#ifdef __cplusplus
		extern "C" {
	#endif

	extern volatile DWORD RTK_TestIdleCounter;
	extern volatile DWORD RTK_TestMainTaskExitCounter;

	DWORD RTK_TestRandom(void);
	void RTK_TestMain(void);

	#ifdef __cplusplus
		}
	#endif

#endif
