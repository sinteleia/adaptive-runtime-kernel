//	VERSION 0.8
#ifndef __BinQue_h
	#define __BinQue_h

	#include "Type.h"
	#include "GenQue.h"
 
	typedef struct TBinaryLenQueHeader{
		TQueHeader QueHeader;
		WORD AndMask;
	}TBinaryLenQueHeader;

	#define IS_BYNARY_LEN_QUE_FULL(Q) ((((Q.QueHeader.InPtr+1)&Q.AndMask)==Q.QueHeader.OutPtr))
	#define IS_BYNARY_LEN_QUE_NOT_FULL(Q) ((((Q.QueHeader.InPtr+1)&Q.AndMask)!=Q.QueHeader.OutPtr))
	#define IS_BYNARY_LEN_QUE_PTR_FULL(Q) ((((Q->QueHeader.InPtr+1)&Q->AndMask)==Q->QueHeader.OutPtr))
	#define IS_BYNARY_LEN_QUE_PTR_NOT_FULL(Q) ((((Q->QueHeader.InPtr+1)&Q->AndMask)!=Q->QueHeader.OutPtr))

	#ifdef __cplusplus
		extern "C" {
	#endif

	WORD BinaryLenQueLen(TBinaryLenQueHeader *Q);
	WORD BinaryLenQueSize(TBinaryLenQueHeader *Q);
	bool BinaryLenQueInc(TBinaryLenQueHeader *Q);
	bool BinaryLenQueDec(TBinaryLenQueHeader *Q);
	// TBinaryLenQueHeader *BinaryLenNewQue(WORD Size); questo é il template per la costruzione delle code reali

	#ifdef __cplusplus
		}
	#endif

#endif