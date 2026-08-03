//	VERSION 0.8
#include "FreeQue.h"

WORD FreeLenQueLen(TFreeLenQueHeader *Q){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 return (Q->QueHeader.InPtr-Q->QueHeader.OutPtr)%Q->MaxSize;
}

WORD FreeLenQueSize(TFreeLenQueHeader *Q){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 return (Q->QueHeader.OutPtr-Q->QueHeader.InPtr-1)%Q->MaxSize;
}