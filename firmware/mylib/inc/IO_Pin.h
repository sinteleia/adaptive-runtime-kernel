//	VERSION 0.8
#ifndef __IO_Pin_h
	#define __IO_Pin_h
	#include "type.h"


	#define PIN_OFF (0xFFFFFFFF)

	#ifdef __cplusplus
		class IO_Pin{
				public:
					BYTE Port;
					BYTE Bit;
					IO_Pin(BYTE P, BYTE B);
					IO_Pin(int EncodedPortPin);
					typedef enum{
						Disabled,
						InputHiZ,
						InputPullUp,
						InputPullDown,
						OutputPushPull,
						OutputOpenDrain,
						OutputOpenDrainPullUp,
						OutputOpenDrainPullDown	// Mi piacerebbe capire a cosa può servire
					}T_PinMode;
					void SetMode(T_PinMode mode);
					bool In();
					void Out(bool Value);
					void SetFunction(DWORD Function);
					bool IsDirectionIn();
		};	
	#endif	
#endif
