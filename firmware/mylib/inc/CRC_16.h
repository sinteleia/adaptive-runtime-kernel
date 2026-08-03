//	VERSION 0.8
#ifndef __CRC_16_h
 #define __CRC_16_h

 #include "Type.h"

 #ifdef __cplusplus
  extern "C" {
 #endif

 WORD CRC_16(BYTE *Buf, DWORD Len);
 void UpgradeCRC(WORD *CRC, BYTE Ch);

 #ifdef __cplusplus
  }
 #endif
    
#endif