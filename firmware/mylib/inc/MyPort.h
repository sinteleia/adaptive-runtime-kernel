//	VERSION 0.8
#include "MyIntrinsics.h"

#ifndef MYPORT_H_
	#define MYPORT_H_
	
	#include "Type.h"
	#include "Port.h"

	#ifdef __cplusplus
		extern "C" {
	#endif

	#define ENCODE_PORT_PIN(port, pin) ((port<<5)|pin)
	#define DECODE_PIN(n) (((n)&0x1Fu) << 0)
	#define DECODE_PORT(n) ((n) >> 5)
	
	#include "type.h"

	static inline bool PinIn(BYTE IObit){
		return((MyPorts[DECODE_PORT(IObit)].IN & (1<<DECODE_PIN(IObit)))!=0);
	}

	static inline void PinOut(BYTE IObit, bool Value){
		if(Value) MyPorts[DECODE_PORT(IObit)].OUTSET=1<<DECODE_PIN(IObit);
		else MyPorts[DECODE_PORT(IObit)].OUTCLR=1<<DECODE_PIN(IObit);
	}
	
	static inline void PinToggle(BYTE IObit){
		MyPorts[DECODE_PORT(IObit)].OUTTGL=1<<DECODE_PIN(IObit);
	}

	typedef enum {DIRECTION_IN, DIRECTION_OUT, DIRECTION_OFF}T_PinDir;

	static inline void PinSetDirection(BYTE IObit, T_PinDir PinDir){
		BYTE Port=DECODE_PORT(IObit);
		BYTE Pin=DECODE_PIN(IObit);
		switch(PinDir){
			case DIRECTION_IN:
				{
					MyPorts[Port].DIRCLR=1<<Pin;
					START_PROTECTION;
					MyPorts[Port].PINCFG[Pin].INEN=1;
					END_PROTECTION;
				}
				break;
			case DIRECTION_OUT:
				{
					MyPorts[Port].DIRSET=1<<Pin;
					START_PROTECTION;
					MyPorts[Port].PINCFG[Pin].INEN=0;
					END_PROTECTION;
				}
				break;
			default:
				{
					MyPorts[Port].DIRCLR=1<<Pin;
					START_PROTECTION;
					MyPorts[Port].PINCFG[Pin].INEN=0;
					END_PROTECTION;
				}
		}	
	}

	static inline bool PinGetDirection(BYTE IObit){
		return((MyPorts[DECODE_PORT(IObit)].DIR & (1<<DECODE_PIN(IObit)))!=0);
	}

	enum T_PinInputMode{PULL_OFF, PULL_UP, PULL_DOWN};

	static inline void PinSetInputMode(BYTE IObit, enum T_PinInputMode PinInputMode){
		BYTE Port=DECODE_PORT(IObit);
		BYTE Pin=DECODE_PIN(IObit);
		switch (PinInputMode) {
			case PULL_UP:
				{
					MyPorts[Port].DIRCLR=1<<Pin;
					START_PROTECTION;
					MyPorts[Port].PINCFG[Pin].PULLEN=1;
					MyPorts[Port].PINCFG[Pin].INEN=1;
					END_PROTECTION;
					MyPorts[Port].OUTSET=1<<Pin;
				}
				break;
			case PULL_DOWN:
				{
					MyPorts[Port].DIRCLR=1<<Pin;
					START_PROTECTION;
					MyPorts[Port].PINCFG[Pin].PULLEN=1;
					MyPorts[Port].PINCFG[Pin].INEN=1;
					END_PROTECTION;
					MyPorts[Port].OUTCLR=1<<Pin;
				}
				break;
			default:
				{
					START_PROTECTION;
					MyPorts[Port].PINCFG[Pin].PULLEN=0;
					END_PROTECTION;
				}
				break;
		}		
	}
	
	#define PIN_OFF (0xFFFFFFFF)
	
	static inline void PinSetFunction(BYTE IObit, DWORD function){
		BYTE Port=DECODE_PORT(IObit);
		BYTE Pin=DECODE_PIN(IObit);
		if (function == PIN_OFF) {
			START_PROTECTION;
			MyPorts[Port].PINCFG[Pin].PMUXEN=0;
			END_PROTECTION;
		}
		else{
			START_PROTECTION;
			MyPorts[Port].PINCFG[Pin].PMUXEN=1;
			END_PROTECTION;
			Pin>>=1;
			if(IObit & 1) {
				// Odd numbered pin
				RESTART_PROTECTION;
				MyPorts[Port].PMUX[Pin].PMUXO=function & 0xf;
				END_PROTECTION;
			} 
			else{
				// Even numbered pin
				RESTART_PROTECTION;
				MyPorts[Port].PMUX[Pin].PMUXE=function & 0xf;
				END_PROTECTION;
			}			
		}	
	}

	 #ifdef __cplusplus
		}
	#endif


#endif