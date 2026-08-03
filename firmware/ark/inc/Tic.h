/*						ARK Project - Adaptive Runtime Kernel

	Module:
		Tic.h

	Purpose:
		Public interface for optional RTK tic object services.

	Description:
		This header declares tic object types and services used to register routines executed from the
		system tick path or from the scheduler path when TIC_OBJs is enabled.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK tic object headers.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#ifndef __Tic_h
 #define __Tic_h
 
 #include "RTK_Config.h"
 #if TIC_OBJs
  #include "Type.h"

  typedef struct T_PtrVector{
   Func Ptr[1];
  }T_PtrVector;

  typedef struct T_HndVector{
   HANDLE Hnd[1];
  }T_HndVector;
  
  #ifdef __cplusplus
   extern "C" {
  #endif

  HANDLE AgganciaTic(Func F);
  HANDLE AgganciaSched(Func F);
  void Sgancia(HANDLE Hnd);
  bool InitTicObjects(WORD MaxSchedRoutines, WORD MaxISR_Routines);
  void DeinitTicObjects(void);
  void TicObjectProcess(void);

  #ifdef __cplusplus
   }
  #endif

 #endif
#endif
