//	VERSION 0.8
#include "Pack16.h"

/*    Pack16
    Codifica una stringa di 3 caratteri in una WORD. Sono ammessi solo i
  caratteri alfabetici maiuscoli, quelli numerici, il punto, lo spazio ed il
  segno -.
*/
WORD Pack16(const char c[3]){
 WORD Paked=0;
 short int i;
 register BYTE C;
 for(i=0; i<3; i++){
  C=c[i];
  if((C>='0')&&(C<='9'))
   C-='0';
  else if((C>='a')&&(C<='z'))
   C=C-'a'+10;
  else if((C>='A')&&(C<='Z'))
   C=C-'A'+10;
  else if(C=='.')
   C=38;
  else if(C==' ')
   C=37;
  else if(C=='-')
   C=36;
  else C=39;	//Carattere non traducibile;
  Paked=Paked*40+C;
 }
 return Paked;
}

/*    UnPack16
    Ritrasforma in caratteri una WORD codificata con la Pack16.
*/
void UnPack16(WORD Paked, char c[3]){
 short int i;
 WORD x;
 for(i=2; i>=0; i--){
  x=Paked%40;
  Paked/=40;
  if(x==39)
   c[i]='?';
  else if(x==38)
   c[i]='.';
  else if(x==37)
   c[i]=' ';
  else if(x==36)
   c[i]='-';
  else if(x<10)
   c[i]=x+'0';
  else
   c[i]=x+'A'-10;
 }
}

/*				Pack32
		Codifica una stringa di 6 caratteri in una DWORD. Sono ammessi solo i caratteri alfabetici maiuscoli, quelli numerici,
	il punto, lo spazio ed il segno -.
*/
DWORD Pack32(const char c[6]){
 DWORD Paked=0;
 short int i;
 register BYTE C;
 for(i=0; i<6; i++){
  C=c[i];
  if((C>='0')&&(C<='9'))
   C-='0';
  else if((C>='a')&&(C<='z'))
   C=C-'a'+10;
  else if((C>='A')&&(C<='Z'))
   C=C-'A'+10;
  else if(C=='.')
   C=38;
  else if(C==' ')
   C=37;
  else if(C=='-')
   C=36;
  else C=39;	//Carattere non traducibile;
  Paked=Paked*40+C;
 }
 return Paked;
}

/*    UnPack16
    Ritrasforma in caratteri una WORD codificata con la Pack16.
*/
void UnPack32(DWORD Paked, char c[6]){
 short int i;
 DWORD x;
 for(i=5; i>=0; i--){
  x=Paked%40;
  Paked/=40;
  if(x==39)
   c[i]='?';
  else if(x==38)
   c[i]='.';
  else if(x==37)
   c[i]=' ';
  else if(x==36)
   c[i]='-';
  else if(x<10)
   c[i]=x+'0';
  else
   c[i]=x+'A'-10;
 }
}

/*				Pack64
		Codifica una stringa di 12 caratteri in una QWORD. Sono ammessi solo i caratteri alfabetici maiuscoli, quelli numerici,
	il punto, lo spazio ed il segno -.
*/
QWORD Pack64(const char c[12]){
	union{
		QWORD AsQ;
		WORD AsW[4];
	}Packed;
	Packed.AsQ=0;
	short int j;
	short int i;
	register BYTE C;
	for(j=0; j<4; j++)
		for(i=0; i<3; i++){
			C=*c++;
			if((C>='0')&&(C<='9'))
				C-='0';
			else if((C>='a')&&(C<='z'))
				C=C-'a'+10;
			else if((C>='A')&&(C<='Z'))
				C=C-'A'+10;
			else if(C=='.')
				C=38;
			else if(C==' ')
				C=37;
			else if(C=='-')
				C=36;
			else C=39;	//Carattere non traducibile;
			Packed.AsW[j]=Packed.AsW[j]*40+C;
		}
	return Packed.AsQ;
}

/*    UnPack64
    Ritrasforma in caratteri una WORD codificata con la Pack16.
*/
void UnPack64(QWORD Packed, char c[12]){
	#define P ((WORD *)&Packed)
	short int j;
	short int i;
	WORD x;
	c-=3;
	for(j=0; j<4; j++){
		c+=6;
		for(i=0; i<3; i++){
			x=P[j]%40;
			P[j]/=40;
			if(x==39) *--c='?';
			else if(x==38) *--c='.';
			else if(x==37) *--c=' ';
			else if(x==36) *--c='-';
			else if(x<10) *--c=x+'0';
			else *--c=x+'A'-10;
		}
	}
}