//	VERSION 0.8
#ifndef __FreeQue_h
	#define __FreeQue_h

	#include "Type.h"
	#include "GenQue.h"

	typedef struct TFreeLenQueHeader{
		TQueHeader QueHeader;
		WORD MaxSize;
	}TFreeLenQueHeader;

	#define IS_FREE_LEN_QUE_FULL(Q) ((((Q.QueHeader.InPtr+1)%Q.MaxSize)==Q.QueHeader.OutPtr))
	#define IS_FREE_LEN_QUE_NOT_FULL(Q) ((((Q.QueHeader.InPtr+1)%Q.MaxSize)!=Q.QueHeader.OutPtr))
	#define IS_FREE_LEN_QUE_PTR_FULL(Q) ((((Q->QueHeader.InPtr+1)%Q->MaxSize)==Q->QueHeader.OutPtr))
	#define IS_FREE_LEN_QUE_PTR_NOT_FULL(Q) ((((Q->QueHeader.InPtr+1)%Q->MaxSize)!=Q->QueHeader.OutPtr))

	#ifdef __cplusplus
		extern "C" {
	#endif

	WORD FreeLenQueLen(TFreeLenQueHeader *Q);
	WORD FreeLenQueSize(TFreeLenQueHeader *Q);

	#ifdef __cplusplus
		}
	#endif

#endif