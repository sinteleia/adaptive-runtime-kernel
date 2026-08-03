/*						Mylib Support Library

	Module:
		type.h

	Purpose:
		Common fixed-width-style scalar types, handles and function pointer aliases.

	Description:
		This header defines the basic scalar aliases and small utility unions used by Mylib and by
		projects built on top of it. The definitions reflect the target compiler and MCU assumptions
		used by this library and are shared by C, C++ and assembly-adjacent interfaces.

	Mylib version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older Mylib type definitions.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __Type_h
	#define __Type_h
	#include "stddef.h"

	typedef unsigned char BYTE;
	typedef unsigned short int WORD;
	typedef unsigned long int DWORD;
	typedef unsigned long int LWORD;
	typedef unsigned long long QWORD;

	#include "stdbool.h"
	typedef BYTE HANDLE;

	typedef	void (*Func)(void);
	typedef void (*FuncPar)(DWORD);
	typedef void (*MultiFuncPar)(DWORD, DWORD, DWORD, DWORD);

	typedef union WordAsByte{
		WORD word;
		struct{
			BYTE B0;
			BYTE B1;
		}byte;
	}WordAsByte;

	typedef union ByteAsBits{
		BYTE byte;
		struct{
			BYTE D0 :1;
			BYTE D1 :1;
			BYTE D2 :1;
			BYTE D3 :1;
			BYTE D4 :1;
			BYTE D5 :1;
			BYTE D6 :1;
			BYTE D7 :1;
		}bit;
	}ByteAsBits;

	typedef union WordAsBits{
		WORD word;
		struct{
			WORD D0 :1;
			WORD D1 :1;
			WORD D2 :1;
			WORD D3 :1;
			WORD D4 :1;
			WORD D5 :1;
			WORD D6 :1;
			WORD D7 :1;
			WORD D8 :1;
			WORD D9 :1;
			WORD D10 :1;
			WORD D11 :1;
			WORD D12 :1;
			WORD D13 :1;
			WORD D14 :1;
			WORD D15 :1;
		}bit;
	}WordAsBits;

	typedef union MultiInt{
		DWORD AsDW;
		long int AsL;
		WORD AsW[2];
		WORD LsW;
		short int AsI[2];
		short int LsI;
		BYTE AsB[4];
		BYTE LsB;
		char AsC[4];
		char LsC;
	}MultiInt;

	#define INVALID_HANDLE (0)

#endif
