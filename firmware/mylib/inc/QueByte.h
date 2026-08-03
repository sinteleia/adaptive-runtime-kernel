//	VERSION 0.8
#ifndef __QueByte_h
 #define __QueByte_h
 
 #include "BinQue.h"
 
 /*    struttura Que:
    !!! N.B. !!! Questa definizione deve essere congruente con quella definita 
  in assembler nel file QueAsm.Inc.
 */
  typedef struct TByteQue{
   TBinaryLenQueHeader BinaryLenQueHeader;
   BYTE Buf[1];
  }TByteQue;
  
 #ifdef __cplusplus
  extern "C" {
 #endif

 TByteQue *NewQue(WORD Size);
 WORD QueGet(TByteQue *Q);
	bool QuePut(TByteQue *Q, const BYTE Ch);
	bool QuePutString(TByteQue *Q, const BYTE *Str);
	const BYTE *QuePutStringPart(TByteQue *Q, const BYTE *Str);
	bool QuePutBuffer(TByteQue *Q, const BYTE *Buf, WORD Len);
 bool QueGetBuffer(TByteQue *Q, BYTE *Buf, WORD Len);
 WORD QueGetBufferPart(TByteQue *Q, void *Buf, WORD Len);
 bool QueFindChar(TByteQue *Q, const BYTE Ch);

 #ifdef __cplusplus
  }
 #endif

#endif
