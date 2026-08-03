/*						Mylib Support Library

	Module:
		general.h

	Purpose:
		General-purpose constants, bit extraction macros and utility declarations.

	Description:
		This header provides common numeric limits, byte/word extraction macros, generic MIN/MAX
		helpers and the SetPriority() declaration used by Mylib and projects built on top of it.

	Mylib version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older Mylib general definitions.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __General_h
 #define __General_h
  
 #include "Type.h"

 #ifndef MAX_32
  #define MAX_32 (LWORD)0x7fffffffL
 #endif
 
 #ifndef MIN_32
  #define MIN_32 (long int)0x80000000L
 #endif

 #ifndef MAX_16
  #define MAX_16 (WORD)0x7fff
 #endif
 
 #ifndef MIN_16
  #define MIN_16 (WORD)0x8000
 #endif

 #ifndef NULL
  #define NULL 0
 #endif

 #ifndef HI
  #define HI(x) ((BYTE)((x)>>8))
 #endif

 #ifndef LO
  #define LO(x) ((BYTE)((x)&0xFF))
 #endif

 #ifndef WHI
  #define WHI(x) ((WORD)((x)>>16))
 #endif

 #ifndef WLO
  #define WLO(x) ((WORD)((x)&0xFFFF))
 #endif

 #ifdef __cplusplus
  extern "C" {
 #endif

 void SetPriority(WORD Vector, WORD Priority);
 		 		
 #ifdef __cplusplus
  }
 #endif

 #ifndef sizeofw
  #define sizeofw(x) ((sizeof(x)+1)>>1) 
 #endif	
 
 #ifndef MAX
  #define MAX(X1, X2) (((X1)>(X2))? X1: X2)
 #endif
 
 #ifndef MIN
  #define MIN(X1, X2) (((X1)<(X2))? X1: X2)
 #endif
 
#endif
