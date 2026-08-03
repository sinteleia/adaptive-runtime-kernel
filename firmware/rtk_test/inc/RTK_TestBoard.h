#ifndef RTK_TEST_BOARD_H_
	#define RTK_TEST_BOARD_H_

	/*									RTK_TestBoard
			This file declares the board support functions used by the RTK test firmware. Each test board must provide
		a hardware-specific implementation of this interface.
	*/

	#include "type.h"
	#include "RTK.h"

	#ifdef __cplusplus
		extern "C" {
	#endif

	void RTK_TestBoardInit(void);							// Initialize the test board GPIOs used by the RTK tests.
	bool RTK_TestBoardConsoleInit(void);					// Initialize the test console transport.
	void RTK_TestBoardWrite(const char *message);			// Write a zero-terminated message to the test console.
	void RTK_TestBoardOut(unsigned id, bool value);			// Drive a test output pin for oscilloscope evidence.
	bool RTK_TestBoardIn(unsigned id);						// Read a test input pin.
	void RTK_TestLedSet(unsigned id, bool on);				// Set a test LED state.
	void RTK_TestLedToggle(unsigned id);					// Toggle a test LED.
	DWORD RTK_TestTimestamp(void);							// Return the system time counter in milliseconds.
	void RTK_TestAsyncTimerStart(Func F, unsigned int Us);	// Start the async test timer and register its callback.
	void RTK_TestAsyncTimerStop(void);						// Stop the async test timer.
	char GetCh(void);										// Wait for one character from the test console.
	bool IsConsolOutputEnded(void);							// Return true when the pending console output queue is empty.
	DWORD RTK_TestRandom(void);								// Torna un numero casuale
	void RTK_TestTimerDelay(int uS);						// Arma un timer per generare una schedulazione ritardata
	extern Flag TimerDelayElapsed;							// Allo scadere del delay setta questo flag

	#ifdef __cplusplus
		}
	#endif

#endif
