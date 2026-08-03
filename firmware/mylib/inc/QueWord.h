//	VERSION 0.8
#ifndef __QueWord_h
 #define __QueWord_h
 
 #include "BinQue.h"

 #ifdef __cplusplus
  typedef struct TWordQue{
   TBinaryLenQueHeader BinaryLenQueHeader;
   WORD Buf[1];
  }TWordQue;
 #else
  typedef struct TWordQue{
   TBinaryLenQueHeader BinaryLenQueHeader;
   WORD Buf[];
  }TWordQue;
 #endif

 #ifdef __cplusplus
  extern "C" {
 #endif

 TWordQue *WordNewQue(WORD Size); 
 bool WordQueGet(TWordQue *Q, WORD *Res);
 bool WordQuePut(TWordQue *Q, WORD Ch);
 bool WordQuePutBuffer(TWordQue *Q, WORD *Buf, WORD Len);
 bool WordQueGetBuffer(TWordQue *Q, WORD *Buf, WORD Len);

 #ifdef __cplusplus
  }
 #endif

#endif