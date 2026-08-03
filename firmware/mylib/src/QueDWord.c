//	VERSION 0.8
#include "QueDWord.h"
#include "mm.h"

TDWordQue *DWordNewQue(WORD Size){
 TDWordQue *Q;
 WORD S=2;
 while (S<=Size) S*=2;
 Q=malloc(4*S+sizeof(TDWordQue));
 if(Q){
  Q->BinaryLenQueHeader.QueHeader.InPtr=Q->BinaryLenQueHeader.QueHeader.OutPtr=0;
  Q->BinaryLenQueHeader.AndMask=S-1;
 }
 return Q;
}

bool DWordQueGet(TDWordQue *Q, DWORD *Res){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 if(Q->BinaryLenQueHeader.QueHeader.InPtr!=Q->BinaryLenQueHeader.QueHeader.OutPtr){
  *Res=Q->Buf[Q->BinaryLenQueHeader.QueHeader.OutPtr];
  Q->BinaryLenQueHeader.QueHeader.OutPtr=(Q->BinaryLenQueHeader.QueHeader.OutPtr+1)&Q->BinaryLenQueHeader.AndMask;
  return true;
 }
 return false;
}

bool DWordQuePut(TDWordQue *Q, DWORD Val){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 WORD Next;
 Q->Buf[Q->BinaryLenQueHeader.QueHeader.InPtr]=Val;
 Next=(Q->BinaryLenQueHeader.QueHeader.InPtr+1)&Q->BinaryLenQueHeader.AndMask;
 if(Next!=Q->BinaryLenQueHeader.QueHeader.OutPtr){
  Q->BinaryLenQueHeader.QueHeader.InPtr=Next;
  return true;
 }
 return false;
}

