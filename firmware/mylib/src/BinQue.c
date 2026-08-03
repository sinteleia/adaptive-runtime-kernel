//	VERSION 0.8
#include "BinQue.h"
#include "mm.h"

WORD BinaryLenQueLen(TBinaryLenQueHeader *Q){
 return (Q->QueHeader.InPtr-Q->QueHeader.OutPtr)&Q->AndMask;
}

WORD BinaryLenQueSize(TBinaryLenQueHeader *Q){
 return (Q->QueHeader.OutPtr-Q->QueHeader.InPtr-1)&Q->AndMask;
}

bool BinaryLenQueInc(TBinaryLenQueHeader *Q){
 WORD Next;
 Next=(Q->QueHeader.InPtr+1)&Q->AndMask;
 if(Next!=Q->QueHeader.OutPtr){
  Q->QueHeader.InPtr=Next;
  return true;
 }
 return false;
}

bool BinaryLenQueDec(TBinaryLenQueHeader *Q){
 if(Q->QueHeader.InPtr!=Q->QueHeader.OutPtr){
  Q->QueHeader.OutPtr=(Q->QueHeader.OutPtr+1)&Q->AndMask;
  return true;
 }
 return false;
}

/*				TBinaryLenQueHeader
		Questa funzione non é pensata per essere istanziata, ma solo come template
	per le implementazioni vere di una coda, dove oltre allo spazio per l'header viene
	allocato anche lo spazio per il payload.
TBinaryLenQueHeader *BinaryLenNewQue(WORD Size){
 TBinaryLenQueHeader *Q;
 WORD S=2;
 while(S<=Size) S*=2;
 Q=(TBinaryLenQueHeader*)malloc(sizeof(TBinaryLenQueHeader));
 if(Q){
  Q->QueHeader.InPtr=Q->QueHeader.OutPtr=0;
  Q->AndMask=S-1;
 }
 return Q;
}
*/