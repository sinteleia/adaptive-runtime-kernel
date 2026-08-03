#ifndef MICRODELAY_H_
	#define MICRODELAY_H_
	#ifdef __cplusplus
		extern "C"{
	#endif
	#include "Type.h"

	#define DWT_CTRL      (*(volatile DWORD *)0xE0001000UL)
	#define DWT_CYCCNT    (*(volatile DWORD *)0xE0001004UL)
	#define CORE_DEMCR    (*(volatile DWORD *)0xE000EDFCUL)

	#define DWT_CTRL_CYCCNTENA    0x00000001UL
	#define DWT_CTRL_NOCYCCNT     0x02000000UL
	#define CORE_DEMCR_TRCENA     0x01000000UL

	bool Init_uS_ToDelay(void);
	void MicroDelay(DWORD uS_ToDelay);

	inline void SubMicroDelay(DWORD CicliToDelay){
		DWORD Old=DWT_CYCCNT;
		while((DWORD)(DWT_CYCCNT-Old)<CicliToDelay);
	}

	#ifdef __cplusplus
		}
	#endif

#endif
