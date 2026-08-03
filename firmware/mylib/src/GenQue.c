//	VERSION 0.8
#include "GenQue.h"

void QuePurge(TQueHeader *Q){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 Q->InPtr=Q->OutPtr=0;
}

