/*						ARK Project - Adaptive Runtime Kernel

	Module:
		Tic.c

	Purpose:
		RTK tic object management implementation.

	Description:
		This module manages the optional RTK tic object tables used to register routines executed from
		the system tick path and from the scheduler path. It allocates and releases the handler tables,
		assigns stable handles, and supports attach/detach operations for ISR and scheduler tic
		routines when TIC_OBJs is enabled.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK tic object implementations.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/
#include "Tic.h"
#if TIC_OBJs

 // #include "Err.h"
 #ifdef ERROR_TRAPPING
 	#include "ErrCode.h"
 #endif
 #include "mm.h"
 #include "MyIntrinsics.h"

 T_PtrVector *ISR;
 T_HndVector *ISR_HndIndex;
 T_PtrVector *Sched;
 T_HndVector *SchedHndIndex;
 WORD NumOfISR;
 WORD MaxISR;
 WORD NumOfSched;
 WORD MaxSched;
 WORD CntSched;
 
 void SganciaTic(HANDLE Hnd);
 void SganciaSched(HANDLE Hnd);
 
 /*    InitTicObjects
     Alloca lo spazio per i tic objects ed inizializza il tutto. Viene passato 
   il numero massimo di TIC e di ISR. La funzione torna false se non trova la
   memoria necessaria.
 */
 bool InitTicObjects(WORD MaxSchedRoutines, WORD MaxISR_Routines){
  NumOfISR=NumOfSched=0;
  ISR=(T_PtrVector *)malloc((MaxSchedRoutines+MaxISR_Routines)*
                            (sizeof(T_HndVector)+sizeof(T_PtrVector)));
  if(ISR){
   Sched=(T_PtrVector *)(&ISR[MaxISR_Routines]);
   ISR_HndIndex=(T_HndVector *)(&Sched[MaxSchedRoutines]);
   SchedHndIndex=(T_HndVector *)(&ISR_HndIndex[MaxISR_Routines]);
   MaxSched=MaxSchedRoutines;
   MaxISR=MaxISR_Routines;
  }
  else{
   #ifdef ERROR_TRAPPING
    CauseError(SCHED_OBJECT_MEMORY_NOT_FOUND);
   #endif
   MaxSched=0;
   MaxISR=0;
   return false;
  }
  return true;
 }

 void DeinitTicObjects(void){
  if(ISR){
   free(ISR);
  }
  ISR=NULL;
  Sched=NULL;
  ISR_HndIndex=NULL;
  SchedHndIndex=NULL;
  NumOfISR=0;
  NumOfSched=0;
  MaxISR=0;
  MaxSched=0;
  CntSched=0;
 }
 
 /*    AgganciaTic
     Inserisce una funzione nella lista dei tic object. La funzione torna
   l'handle se ci riesce, INVALID_HANDLE se non c'� posto.
 */
 HANDLE AgganciaTic(Func F){
  if(NumOfISR==0){
   for(WORD i=0; i<MaxISR; i++)
    ISR_HndIndex->Hnd[i]=i+1;
  }
  if(NumOfISR<MaxISR){
   ISR->Ptr[NumOfISR]=F;
   NumOfISR++;
   return ISR_HndIndex->Hnd[NumOfISR-1];
  }
  #ifdef ERROR_TRAPPING
   CauseError(AGGANCIA_TIC_ERROR_NO_SPACE);
  #endif
  return INVALID_HANDLE; 
 }

 /*     SganciaTic
    Disinserisce una funzione di handle passato dalla lista dei tic object.
   Questa funzione non � pubblica in quanto viene invocata dalla pi� generica
   Sgancia, che sgancia un handle qualsiasi dalla lista corretta.
    Per consentire di agganciare e sganciare le tic routines anche dentro un
   interrupt o dentro un tic, potrebbe essere modificata come segue:
   if(Tic in corso)
   {
    inc RoutinesDaSganciare;
    Ptr to routine=placeholder routine;
   }                         
   else
    SganciaTic normale.
    
   Nel tic, poi, all'inizio:
   while RoutinesDaSganciare
   {
    Sgancia la prima con il placeholder
    dec RoutinesDaSganciare
   }
 */
 void SganciaTic(HANDLE Hnd){
  if(NumOfISR){
   NumOfISR--;
   for(WORD i=0; i<NumOfISR; i++){
    if(ISR_HndIndex->Hnd[i]==Hnd){
     START_PROTECTION;
     for(; i<NumOfISR; i++){
      ISR_HndIndex->Hnd[i]=ISR_HndIndex->Hnd[i+1];
      ISR[i]=ISR[i+1];
     }
     ISR_HndIndex->Hnd[i]=Hnd;
     END_PROTECTION;
     break;
    }
   } 
  }
  #ifdef ERROR_TRAPPING
   else
    CauseError(ERROR_FREE_HANDLER);
  #endif
 }

 /*    AgganciaSched
     Inserisce una funzione nella lista deglis sched object. La funzione torna
   l'handle se ci riesce, INVALID_HANDLE se non c'� posto.
 */
 HANDLE AgganciaSched(Func F){
  if(NumOfSched==0){
   for(WORD i=0; i<MaxSched; i++)
    SchedHndIndex->Hnd[i]=i+MaxISR+1;
  }
  if(NumOfSched<MaxSched){
   Sched->Ptr[NumOfSched]=F;
   NumOfSched++;
   return SchedHndIndex->Hnd[NumOfSched-1];
  }
  #ifdef ERROR_TRAPPING
   CauseError(AGGANCIA_SCHED_ERROR_NO_SPACE);
  #endif
 return INVALID_HANDLE; 
 } 

 /*    SganciaSched
    Disinserisce una funzione di handle passato dalla lista degli sched
   object. Questa funzione non � pubblica in quanto viene invocata dalla
   pi� generica Sgancia, che sgancia un handle qualsiasi dalla lista
   corretta.
 */
 void SganciaSched(HANDLE Hnd){
  if(NumOfSched){
   NumOfSched--;
   for(WORD i=0; i<NumOfSched; i++){
    if(SchedHndIndex->Hnd[i]==Hnd){
     START_PROTECTION;
     for(; i<NumOfSched; i++){
      SchedHndIndex->Hnd[i]=SchedHndIndex->Hnd[i+1];
      Sched[i]=Sched[i+1];
     }
     SchedHndIndex->Hnd[i]=Hnd;
     END_PROTECTION;    // Ripristina lo stato originale di IE
     break;
    }
   } 
  }
 } 

 /*    Sgancia
    Disinserisce una funzione di handle passato dalla lista degli sched
   object o dei tic object. 
 */
 void Sgancia(HANDLE Hnd){
  if(Hnd){
   if(Hnd<=MaxISR)
    SganciaTic(Hnd);
   else if(Hnd<=MaxISR+MaxSched)
    SganciaSched(Hnd);
   #ifdef ERROR_TRAPPING
    else
     CauseError(ERROR_INVALID_HANDLER);
   #endif
  }  
 }

 /*    TicObjectProcess
     Effettua l'elaborazione periodica dei tic objects.
 */
 void TicObjectProcess(void){
  for(WORD i=0; i<NumOfISR; i++)
   ISR->Ptr[i]();
  #if CONSTANT_SCHEDULING_TIME
   if(CntSched++>=MaxSched)
    CntSched=0;
   if(CntSched<NumOfSched)
    Sched->Ptr[CntSched]();
  #else
   if(NumOfSched){
    if(++CntSched>=NumOfSched)
     CntSched=0;
    Sched->Ptr[CntSched]();
   }
  #endif   
 }

#endif
