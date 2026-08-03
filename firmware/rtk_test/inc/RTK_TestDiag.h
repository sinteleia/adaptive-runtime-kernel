#ifndef RTK_TEST_DIAG_H_
	#define RTK_TEST_DIAG_H_

	#include "type.h"

	#ifdef __cplusplus
		extern "C" {
	#endif

	void RTK_TestDiagInit(void);
	void RTK_TestDiagBeginCycle(unsigned Cycle);
	void RTK_TestDiagEndCycle(void);
	void RTK_TestDiagSetCompactProgress(bool Enabled);
	void RTK_TestProgress(void);
	void RTK_TestLog(const char *message);
	void RTK_TestPrintf(const char *format, ...);
	void RTK_TestPass(const char *test_id);
	void RTK_TestFail(const char *test_id, const char *reason);
	void RTK_TestFatal(const char *reason);
	void RTK_TestSummary(void);

	#ifdef __cplusplus
		}
	#endif

#endif
