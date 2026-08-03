//	VERSION 0.8
#include "WFill.h"

void Fill(BYTE P, void *Buf, WORD Len){
	while(Len--)
		*((char *)Buf++)=P;
}

void WordFill(WORD P, WORD *Buf, WORD Len){
	while(Len--)
		*(Buf++)=P;
}

void DWordFill(DWORD P, DWORD *Buf, WORD Len){
	while(Len--)
		*(Buf++)=P;
}

void QWordFill(QWORD P, QWORD *Buf, WORD Len){
	while(Len--)
		*(Buf++)=P;
}
