//	VERSION 0.8
#include "IO_Pin.h"
#include "MyIntrinsics.h"
#include "MyPort.h"

IO_Pin::IO_Pin(BYTE P, BYTE B){
	Port=P;
	Bit=B;
}

IO_Pin::IO_Pin(int EncodedPortPin){
	Port=DECODE_PORT(EncodedPortPin);
	Bit=DECODE_PIN(EncodedPortPin);
}

void IO_Pin::SetMode(T_PinMode mode){
	switch(mode){
		case T_PinMode::InputHiZ:
			{
				START_PROTECTION;
				MyPorts[Port].PINCFG[Bit].PULLEN=0;
				END_PROTECTION;
				MyPorts[Port].DIRCLR=1<<Bit;
				RESTART_PROTECTION;
				MyPorts[Port].PINCFG[Bit].INEN=1;
				END_PROTECTION;
			}
			break;
		case T_PinMode::InputPullUp:
			{
				register DWORD Msk=1<<Bit;
				START_PROTECTION;
				MyPorts[Port].PINCFG[Bit].PULLEN=1;
				END_PROTECTION;
				MyPorts[Port].OUTSET=Msk;
				MyPorts[Port].DIRCLR=Msk;
				RESTART_PROTECTION;
				MyPorts[Port].PINCFG[Bit].INEN=1;
				END_PROTECTION;
			}
			break;
		case T_PinMode::InputPullDown:
			{
				START_PROTECTION;
				MyPorts[Port].PINCFG[Bit].PULLEN=1;
				END_PROTECTION;
				register DWORD Msk=1<<Bit;
				MyPorts[Port].OUTCLR=Msk;
				MyPorts[Port].DIRCLR=Msk;
				RESTART_PROTECTION;
				MyPorts[Port].PINCFG[Bit].INEN=1;
				END_PROTECTION;
			}
			break;
		case T_PinMode::Output:
			{
				MyPorts[Port].DIRSET=1<<Bit;
				START_PROTECTION;
				MyPorts[Port].PINCFG[Bit].INEN=0;
				END_PROTECTION;
			}
			break;
		default:
			{
				MyPorts[Port].DIRCLR=1<<Bit;
				START_PROTECTION;
				MyPorts[Port].PINCFG[Bit].INEN=0;
				END_PROTECTION;
			}
	}	
}

bool IO_Pin::In(){
	return((MyPorts[Port].IN & (1<<Bit))!=0);
}

void IO_Pin::Out(bool Value){
	if(Value)
		MyPorts[Port].OUTSET=1<<Bit;
	else
		MyPorts[Port].OUTCLR=1<<Bit;
}

void IO_Pin::SetFunction(DWORD Function){		
	if (Function == PIN_OFF) {
		START_PROTECTION;
		MyPorts[Port].PINCFG[Bit].PMUXEN=0;
		END_PROTECTION;
	}
	else{
		START_PROTECTION;
		MyPorts[Port].PINCFG[Bit].PMUXEN=1;
		END_PROTECTION;
		if(Bit & 1) {
			// Odd numbered pin
			RESTART_PROTECTION;
			MyPorts[Port].PMUX[Bit>>=1].PMUXO=Function & 0xf;
			END_PROTECTION;
		}
		else{
			// Even numbered pin
			RESTART_PROTECTION;
			MyPorts[Port].PMUX[Bit>>=1].PMUXE=Function & 0xf;
			END_PROTECTION;
		}
	}
}

bool IO_Pin::IsDirectionIn(){
	return((MyPorts[Port].DIR & (1<<Bit))==0);
}

