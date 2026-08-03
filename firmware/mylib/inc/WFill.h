//	VERSION 0.8
#ifndef __WFill_h
	#define __WFill_h

	#include "Type.h"

	#ifdef __cplusplus
		extern "C" {
	#endif

 void Fill(BYTE P, void *Buf, WORD Len);
	void WordFill(WORD P, WORD *Buf, WORD Len);
	void DWordFill(DWORD P, DWORD *Buf, WORD Len);
	void QWordFill(QWORD P, QWORD *Buf, WORD Len);

	#ifdef __cplusplus
		}
	#endif

#endif
