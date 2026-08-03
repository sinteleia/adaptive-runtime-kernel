//	VERSION 0.8
#ifndef __GenQue_h
 #define __GenQue_h

 #include "Type.h"

 typedef struct TQueHeader{
  WORD InPtr;
  WORD OutPtr;
 }TQueHeader;

 #define IS_QUE_EMPTY(Q) ((Q.InPtr==Q.OutPtr))
 #define IS_QUE_NOT_EMPTY(Q) ((Q.InPtr!=Q.OutPtr))
 #define IS_QUE_PTR_EMPTY(Q) ((Q->InPtr==Q->OutPtr))
 #define IS_QUE_PTR_NOT_EMPTY(Q) ((Q->InPtr!=Q->OutPtr))

 #ifdef __cplusplus
  extern "C" {
 #endif

 void QuePurge(TQueHeader  *Q);

 #ifdef __cplusplus
  }
 #endif

#endif