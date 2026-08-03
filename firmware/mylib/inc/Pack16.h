//	VERSION 0.8
#ifndef __PACK_16_H
	#define __PACK_16_H

	#include "Type.h"

	/*					PRINCIPIO DI FUNZIONAMENTO

			Routines per il pack e l'unpack di un ridotto set di caratteri su WORD a sedici bit. I caratteri comprendono i numeri da
		0 a 9, le lettere maiuscole da A a Z, lo spazio, il punto, il trattino ed il punto interrogativo. Ogni altro ASCII viene
		tradotto in punto interrogativo durante il packing.

			'0'..'9'    <--> 0..9
			'A'..'Z'    <--> 10..35
			'-'         <--> 36
			' '         <--> 37
			'.'         <--> 38
			Altri ASCII  --> 39
			'?'         <--  39
	*/

	#define ALL_INVALID_16 0xF9FFU
	#define ALL_INVALID_32 0xF423FFFFU
	#define ALL_INVALID_64 16777215999999999999U

	#ifdef __cplusplus
		extern "C" {
	#endif

	WORD Pack16(const char c[3]);
	void UnPack16(WORD Paked, char c[3]);
	DWORD Pack32(const char c[6]);
	void UnPack32(DWORD Paked, char c[6]);
	QWORD Pack64(const char c[12]);
	void UnPack64(QWORD Paked, char c[12]);

	#ifdef __cplusplus
		}
	#endif

#endif

