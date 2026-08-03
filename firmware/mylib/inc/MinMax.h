/*						Mylib Support Library

	Module:
		MinMax.h

	Purpose:
		Helpers to take the minimum or maximum between two values.

	Description:
		This header provides generic MIN/MAX macros when they are not already defined and declares
		helper functions that return the minimum or maximum between two signed or unsigned values.

	Mylib version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older Mylib utility headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __MinMax_h
 #define __MinMax_h
 
 #include "Type.h"
 #ifndef MAX
	 #define MAX(X1, X2) (((X1)>(X2))? X1: X2)
 #endif
 
 #ifndef MIN
	 #define MIN(X1, X2) (((X1)<(X2))? X1: X2)
 #endif

 #ifdef __cplusplus
  extern "C" {
 #endif

 short int Min(short int a, short int b);
 short int Max(short int a, short int b);
 WORD UMin(WORD a, WORD b);
 WORD UMax(WORD a, WORD b);
 
 #ifdef __cplusplus
  }
 #endif

#endif
