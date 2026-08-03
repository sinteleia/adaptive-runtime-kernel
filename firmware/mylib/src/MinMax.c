/*						Mylib Support Library

	Module:
		MinMax.c

	Purpose:
		Signed and unsigned integer min/max utility implementation.

	Description:
		This module implements simple min/max helpers for signed short integers and unsigned WORD
		values, matching the declarations provided by MinMax.h.

	Mylib version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older Mylib utility functions.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include "MinMax.h"

/*    Min
    Torna il minore di 2 interi con segno.
*/
short int Min(short int a, short int b){
 return a<b? a: b;
}

/*    Max
    Torna il maggiore di 2 interi con segno.
*/
short int Max(short int a, short int b){
 return a>b? a: b;
}

/*    UMin
    Torna il minore di 2 interi senza segno.
*/
WORD UMin(WORD a, WORD b){
 return a<b? a: b;
}

/*    UMax
    Torna il maggiore di 2 interi senza segno.
*/
WORD UMax(WORD a, WORD b){
 return a>b? a: b;
}
