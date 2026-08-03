//	VERSION 0.8
#ifndef __QueDWord_h
 #define __QueDWord_h
 
 #include "BinQue.h"

 #ifdef __cplusplus
  typedef struct TDWordQue{
   TBinaryLenQueHeader BinaryLenQueHeader;
   DWORD Buf[1];
  }TDWordQue;
 #else
  typedef struct TDWordQue{
   TBinaryLenQueHeader BinaryLenQueHeader;
   DWORD Buf[1];
  }TDWordQue;
 #endif

#ifdef __cplusplus
  extern "C" {
 #endif

 TDWordQue *DWordNewQue(WORD Size); 
 bool DWordQueGet(TDWordQue *Q, DWORD *Res);
 bool DWordQuePut(TDWordQue *Q, DWORD Ch);

 #ifdef __cplusplus
  }
 #endif

#endif