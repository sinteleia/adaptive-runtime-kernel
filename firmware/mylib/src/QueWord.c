//	VERSION 0.8
#include "QueWord.h"
#include "mm.h"

TWordQue *WordNewQue(WORD Size){
 TWordQue *Q;
 WORD S=2;
 while (S<=Size) S*=2;
 Q=(TWordQue*)malloc(2*S+sizeof(TWordQue));
 if(Q){
  Q->BinaryLenQueHeader.QueHeader.InPtr=Q->BinaryLenQueHeader.QueHeader.OutPtr=0;
  Q->BinaryLenQueHeader.AndMask=S-1;
 }
 return Q;
}

bool WordQueGet(TWordQue *Q, WORD *Res){
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

bool WordQuePut(TWordQue *Q, WORD Val){
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

bool WordQuePutBuffer(TWordQue *Q, WORD *Buf, WORD Len){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 if(BinaryLenQueSize((TBinaryLenQueHeader *)Q)<Len)
  return false;
 for(unsigned int i=0; i<Len; i++)
  WordQuePut(Q, Buf[i]);
 return true;
}

bool WordQueGetBuffer(TWordQue *Q, WORD *Buf, WORD Len){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 if(BinaryLenQueLen((TBinaryLenQueHeader *)Q)>=Len){
  for(WORD i=0; i<Len; i++)
   WordQueGet(Q, &Buf[i]);
  return true;
 }
 else
  return false;
}

