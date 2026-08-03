//	VERSION 0.8
#include "QueByte.h"
#include "general.h"
#include "mm.h"
#include "string.h"

bool QuePut(TByteQue *Q, const BYTE Ch){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 WORD Next;
 Q->Buf[Q->BinaryLenQueHeader.QueHeader.InPtr]=Ch;
 Next=(Q->BinaryLenQueHeader.QueHeader.InPtr+1)&Q->BinaryLenQueHeader.AndMask;
 if(Next!=Q->BinaryLenQueHeader.QueHeader.OutPtr){
  Q->BinaryLenQueHeader.QueHeader.InPtr=Next;
  return true;
 }
 return false;
}

WORD QueGet(TByteQue *Q){
 BYTE Res;
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 if(Q->BinaryLenQueHeader.QueHeader.InPtr!=Q->BinaryLenQueHeader.QueHeader.OutPtr){
  Res=Q->Buf[Q->BinaryLenQueHeader.QueHeader.OutPtr];
  Q->BinaryLenQueHeader.QueHeader.OutPtr=(Q->BinaryLenQueHeader.QueHeader.OutPtr+1)&Q->BinaryLenQueHeader.AndMask;
  return Res|0xFF00;
 }
 return 0;
}

TByteQue *NewQue(WORD Size){
 TByteQue *Q;
 WORD S=2;
 while(S<=Size) S*=2;
 Q=(TByteQue *)malloc(S+sizeof(TByteQue));
 if(Q){
  Q->BinaryLenQueHeader.QueHeader.InPtr=Q->BinaryLenQueHeader.QueHeader.OutPtr=0;
  Q->BinaryLenQueHeader.AndMask=S-1;
 }
 return Q;
}

bool QuePutString(TByteQue *Q, const BYTE *Str){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 return(QuePutBuffer(Q, Str, strlen((char *)Str)));
}

const BYTE *QuePutStringPart(TByteQue *Q, const BYTE *Str){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 WORD i;
 WORD L=strlen((char *)Str);
 for(i=0; i<L; i++){
  if(QuePut(Q, *Str))
   Str++;
  else
   return Str;
 }
 return NULL;
}

bool QuePutBuffer(TByteQue *Q, const BYTE *Buf, WORD Len){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 WORD i;
 if(BinaryLenQueSize((TBinaryLenQueHeader *)Q)<Len)
  return false;
 for(i=0; i<Len; i++)
  QuePut(Q, Buf[i]);
 return true;
}

bool QueGetBuffer(TByteQue *Q, BYTE *Buf, WORD Len){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
 WORD i;
 if(BinaryLenQueLen((TBinaryLenQueHeader *)Q)>=Len){
  for(i=0; i<Len; i++)
   Buf[i]=LO(QueGet(Q));
  return true;
 }
 else
  return false;
}

WORD QueGetBufferPart(TByteQue *Q, void *Buf, WORD Len){
	#ifdef DEBUG
		if(Q==0) __asm("BKPT #0\n") ; // Break into the debugger
	#endif
	WORD Ch;
	WORD N=0;
	while(N<Len)
		if((Ch=QueGet(Q))!=0)
			((BYTE *)Buf)[N++]=LO(Ch);
		else
			break;
	return N;
}

