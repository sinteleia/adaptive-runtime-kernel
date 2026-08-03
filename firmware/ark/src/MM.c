/*						ARK Project - Adaptive Runtime Kernel

	Module:
		MM.c

	Purpose:
		Heap memory manager implementation used by ARK/RTK applications.

	Description:
		This module implements initialization, allocation, release and heap consistency checks for the
		ARK memory manager. It manages a linker-provided or caller-provided heap as a list of physical
		blocks, supports configurable payload alignment and optional diagnostics, and protects heap
		operations using the mode selected in MM.cfg.

	ARK version:
		1.0

	File revision:
		1.0

	Origin:
		Derived from older RTK/MM versions.

	Author:
		Paolo Rozzi

	Reviewer:
		---

*/

#include "MM.h"
#include "Sem.h"
#include "string.h"
#include "ErrCode.h"

#if (MALLOC_INTERRUPT_PROTECT + MALLOC_SCHEDULER_PROTECT + MALLOC_SEMAPHORE_PROTECT) != 1
	#error "Select exactly one malloc protection mode"
#endif

#if MALLOC_INTERRUPT_PROTECT
	#include "MyIntrinsics.h"
	#define MM_START_PROTECTION START_PROTECTION
	#define MM_RESTART_PROTECTION RESTART_PROTECTION
	#define MM_END_PROTECTION END_PROTECTION
#elif MALLOC_SCHEDULER_PROTECT
	#include "Sched.h"
	#define MM_START_PROTECTION uint32_t MM_SchedulerLock=RTK_SchedulerLock()
	#define MM_RESTART_PROTECTION MM_SchedulerLock=RTK_SchedulerLock()
	#define MM_END_PROTECTION RTK_SchedulerUnlock(MM_SchedulerLock)
#elif MALLOC_SEMAPHORE_PROTECT
	#include "RTK_Wait.h"
	Semaphore HeapSem;
	#define MM_START_PROTECTION CheckAndWaitForSem(&HeapSem)
	#define MM_RESTART_PROTECTION CheckAndWaitForSem(&HeapSem)
	#define MM_END_PROTECTION RELEASE_SEM(HeapSem)
#endif

#define BYTE_PTR(X) ((BYTE *)((void *)X))

#if PAYLOAD_ALIGN_4
	#define BLOCK_ALIGN 4
#else
	#define BLOCK_ALIGN 8
#endif

extern BYTE _sheap;
extern BYTE _eheap;
#define HeapStart & _sheap
#define HeapEnd & _eheap
#define BLOCK_ALIGN_SUM (BLOCK_ALIGN-1)
#define BLOCK_ALIGN_MASK (~BLOCK_ALIGN_SUM)
#define HP_LEN ((HeapEnd-HeapStart)&BLOCK_ALIGN_MASK)

/*						S_BlkHeader
		Block descriptor structure.

		Every block, whether allocated or not, has a header that must be a multiple of the block’s alignment (4 or 8 bytes). 
	For all block types, this header contains the length, followed—for allocated blocks—by the allocated size (specified during 
	the `malloc` call) and, for unallocated blocks, by a pointer to the next free block. 
*/
struct S_BlkHeader{
	T_Len BlkLen;					// Phisical size of the memory block
	union{
		T_Len AllocLen;				// Requested size when allocated
		struct S_BlkHeader *Next;	// Pointer to the next free block when not unallocated 
	};
};

typedef struct S_BlkHeader T_BlkHeader;

#define MIN_FREEBLOCK_SIZE ((sizeof(T_BlkHeader)+BLOCK_ALIGN_SUM)&BLOCK_ALIGN_MASK)

T_BlkHeader *FirstFreeBlk;
T_BlkHeader *FirstHeapBlk;

#if BLOCK_COUNTER
	DWORD TotalAllocExecuted;
	short int BlockCounter;
#endif

/*					minit
	Purpose:
		Initializes the default heap memory block.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This routine aligns the first heap block to BLOCK_ALIGN and prepares the initial free block list.
		When semaphore protection is enabled, it initializes the heap semaphore as free.
	Input:
		None. This function use global defines HeapStart and HeapLen settled by the linker to define heap position and size
	Output:
		None.
	Note:
		Every heap block and corrispective payload as to be aligned as specified in BLOCK_ALIGN.
*/
void minit(void){
	int l=HP_LEN;
	FirstFreeBlk=(T_BlkHeader *)HeapStart;
	while(((int)FirstFreeBlk)&BLOCK_ALIGN_SUM){
		FirstFreeBlk=(T_BlkHeader *)(BYTE_PTR(FirstFreeBlk)+1);
		l--;
	}
	FirstHeapBlk=FirstFreeBlk;
	FirstFreeBlk->BlkLen=l;
	FirstFreeBlk->Next=NULL;
	#if MALLOC_SEMAPHORE_PROTECT
		HeapSem=SEM_FREE;
	#endif
	#if BLOCK_COUNTER
		BlockCounter=0;
	#endif

}

/*					MinitFromBlock
	Purpose:
		Initializes the heap from a caller-provided memory block.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This routine aligns the first heap block to BLOCK_ALIGN and prepares the initial free block list.
		When semaphore protection is enabled, it initializes the heap semaphore as free.
	Input:
		Ad: start address of the memory block to use as heap.
		l: size of the memory block in bytes.
	Output:
		None.
*/
void MinitFromBlock(void *Ad, T_Len l){
	FirstFreeBlk=(T_BlkHeader *)Ad;
	while(((int)FirstFreeBlk)&BLOCK_ALIGN_SUM){
		FirstFreeBlk=(T_BlkHeader *)(BYTE_PTR(FirstFreeBlk)+1);
		l--;
	}
	l&=BLOCK_ALIGN_MASK;
	FirstHeapBlk=FirstFreeBlk;
	FirstFreeBlk->BlkLen=l;
	FirstFreeBlk->Next=NULL;
	#if MALLOC_SEMAPHORE_PROTECT
		HeapSem=SEM_FREE;
	#endif
	#if BLOCK_COUNTER
		BlockCounter=0;
	#endif
}

/*					MM_Malloc
	Purpose:
		Allocates a payload block from the heap.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This routine searches the free list for a block large enough for the requested payload. The requested size
		is rounded up to BLOCK_ALIGN, and the free block is split when enough space remains for a valid free block.
		Heap access is protected according to the configured malloc protection mode.
	Input:
		size: requested payload size in bytes.
	Output:
		Pointer to the allocated payload, or NULL if no suitable block is available.
	Notes:
		- The returned payload address is aligned to BLOCK_ALIGN. When MALLOC_GUARD is enabled, one guard byte is
		  stored after the requested payload;
		- Malloc can terminate with a global error if MALLOC_GUARD or MALLOC_TESTare enabled and a fatal error is detected.
*/
void *MM_Malloc(size_t size){
	if(size==0) return NULL;
	#if MALLOC_GUARD
		size_t AllocLen=size;
		size+=1;
	#endif
	//	Check dell'heap ad ogni alloc, da usarsi solo per test
	#if MALLOC_TEST
		T_Len HD, MBD;
		int NOFB, NOAB;
		T_HeapStatus HS=HeapStatus(&HD, &MBD, &NOFB, &NOAB);
		if((HS!=HeapOk)&&(HS!=BlockNumberError))	// Se si é verificato un errore
			CauseError(RTK_FATAL_HEAP_CORRUPTION+(int)HS);
	#endif
	size=(size+BLOCK_ALIGN_SUM)&BLOCK_ALIGN_MASK;		// Size deve essere arrotondato al primo block align successivo
	MM_START_PROTECTION;
	T_BlkHeader **PrevPtr=&FirstFreeBlk;
	T_BlkHeader *Cnt=FirstFreeBlk;
	while(Cnt){	// Cerca il primo blocco con dimensioni bastanti
		// if(Cnt->Len>size){	// Trovato!
		if(Cnt->BlkLen >= size + BLOCK_ALIGN){	// Trovato! PATCH 17/4/26
			// Guarda se possiamo usarne solo una parte
			if(Cnt->BlkLen>size+BLOCK_ALIGN+MIN_FREEBLOCK_SIZE){
				// e nel caso Spezza il blocco
				T_BlkHeader *N=((void *)Cnt)+size+BLOCK_ALIGN;
				N->Next=Cnt->Next;
				N->BlkLen=Cnt->BlkLen-size-BLOCK_ALIGN;
				Cnt->BlkLen=size+BLOCK_ALIGN;
				Cnt->Next=N;
			}
			*PrevPtr=Cnt->Next;

			#if BLOCK_COUNTER
				TotalAllocExecuted++;
				BlockCounter++;
			#endif

			MM_END_PROTECTION;

			#if MALLOC_GUARD
				Cnt->AllocLen=AllocLen;
				BYTE *B=(BYTE *)Cnt;
				B[AllocLen+BLOCK_ALIGN]=0Xa5;
			#endif

			#if MALLOC_TEST
				T_HeapStatus HS=HeapStatus(&HD, &MBD, &NOFB, &NOAB);
				if((HS!=HeapOk)&&(HS!=BlockNumberError))	// Se si é verificato un errore
					CauseError(RTK_FATAL_HEAP_CORRUPTION+(int)HS);
			#endif

			return ((void *)Cnt)+BLOCK_ALIGN;
		}
		PrevPtr=&(Cnt->Next);
		Cnt=Cnt->Next;
	}
	MM_END_PROTECTION;
	return NULL;
}

void *malloc(size_t size) __attribute__((alias("MM_Malloc")));

/*					MM_Calloc
	Purpose:
		Allocates an array from the heap and clears it to zero.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This routine implements the standard calloc() behavior using malloc() and memset().
	Input:
		number: number of array elements to allocate.
		size: size of each array element in bytes.
	Output:
		Pointer to the zero-filled allocated payload, or NULL if the allocation fails or the total size overflows.
	Notes:
		If either number or size is zero, this routine returns NULL because malloc(0) returns NULL in this memory manager.
*/
void *MM_Calloc(size_t number, size_t size){
	if((number!=0)&&(size>((size_t)-1)/number))
		return NULL;

	size_t total=number*size;
	void *Blk=MM_Malloc(total);
	if(Blk==NULL)
		return NULL;

	memset(Blk, 0, total);
	return Blk;
}

void *calloc(size_t number, size_t size) __attribute__((alias("MM_Calloc")));

/*					MM_Free
	Purpose:
		Releases a previously allocated payload block back to the ARK heap.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This routine reinserts the block in the free list while keeping the list ordered by address. Heap access is
		protected according to the configured malloc protection mode.
	Input:
		Blk: pointer returned by malloc(), or NULL.
	Output:
		None.
	Notes:
		- Adjacent free blocks are merged when possible. When MALLOC_GUARD is enabled, the guard byte is checked before
		  releasing the block.
		- Malloc can terminate with a global error if MALLOC_GUARD or MALLOC_TESTare enabled and a fatal error is detected.
*/
void MM_Free(void *Blk){
	if(Blk){
		register T_BlkHeader *ToInsert=(T_BlkHeader *)(BYTE_PTR(Blk)-BLOCK_ALIGN);
		register T_BlkHeader *Cnt;

		#if MALLOC_GUARD
			// BYTE *B=(BYTE *)ToInsert;
			if(((BYTE *)Blk)[ToInsert->AllocLen]!=0Xa5)
				CauseError(RTK_FATAL_HEAP_GUARD_ERROR);
		#endif

		#if MALLOC_TEST
			T_Len HD, MBD;
			int NOFB, NOAB;
			T_HeapStatus HS=HeapStatus(&HD, &MBD, &NOFB, &NOAB);
			if((HS!=HeapOk)&&(HS!=BlockNumberError))	// Se si é verificato un errore
				CauseError(RTK_FATAL_HEAP_CORRUPTION+(int)HS);
		#endif

		MM_START_PROTECTION;

		#if BLOCK_COUNTER
			BlockCounter--;
		#endif

		if(FirstFreeBlk==NULL){
			ToInsert->Next=NULL;
			FirstFreeBlk=ToInsert;
		}
		else if(FirstFreeBlk>ToInsert){
			ToInsert->Next=FirstFreeBlk;
			FirstFreeBlk=ToInsert;
			if((T_BlkHeader *)(BYTE_PTR(ToInsert)+ToInsert->BlkLen)==ToInsert->Next){
				ToInsert->BlkLen+=ToInsert->Next->BlkLen;
				ToInsert->Next=ToInsert->Next->Next;
			}
		}
		else{
			Cnt=FirstFreeBlk;
			while((Cnt->Next!=0)&&(Cnt->Next<ToInsert))
				Cnt=Cnt->Next;
			ToInsert->Next=Cnt->Next;
			Cnt->Next=ToInsert;
			if((T_BlkHeader *)(BYTE_PTR(ToInsert)+ToInsert->BlkLen)==ToInsert->Next){
				ToInsert->BlkLen+=ToInsert->Next->BlkLen;
				ToInsert->Next=ToInsert->Next->Next;
			}
			if((T_BlkHeader *)(BYTE_PTR(Cnt)+Cnt->BlkLen)==Cnt->Next){
				Cnt->BlkLen+=Cnt->Next->BlkLen;
				Cnt->Next=Cnt->Next->Next;
			}
		}
		MM_END_PROTECTION;
	}

	#if MALLOC_TEST
		T_Len HD, MBD;
		int NOFB, NOAB;
		T_HeapStatus HS=HeapStatus(&HD, &MBD, &NOFB, &NOAB);
		if((HS!=HeapOk)&&(HS!=BlockNumberError))	// Se si é verificato un errore
			CauseError(RTK_FATAL_HEAP_CORRUPTION+(int)HS);
	#endif
}

void free(void *Blk) __attribute__((alias("MM_Free")));

/*					MM_UsableSize
	Purpose:
		Returns the usable payload size of an allocated heap block.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This routine reads the allocation header located immediately before the payload pointer.
	Input:
		Blk: pointer returned by malloc(), or NULL.
	Output:
		Usable payload size in bytes, or 0 when Blk is NULL.
	Note:
		If MALLOC_GUARD is set to 0, the function returns the size of the block’s payload, regardless of the size requested
		from malloc. If MALLOC_GUARD is set to 1, the function returns the size requested from malloc, stored in AllocLen.
*/
T_Len MM_UsableSize(void *Blk){
	if(Blk==NULL) return 0;
	T_BlkHeader *Cnt=(T_BlkHeader *)(BYTE_PTR(Blk)-BLOCK_ALIGN);
	#if MALLOC_GUARD
		return Cnt->AllocLen;
	#else
		return Cnt->BlkLen-BLOCK_ALIGN;
	#endif
}

/*					NumOfMaxAllocableBytes
	Purpose:
		Returns the largest payload size that can currently be allocated.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This routine scans the free list and subtracts the allocation header size from the largest free block.
	Input:
		None.
	Output:
		Largest currently allocable payload size in bytes.
	Notes:
		This routine does not take a heap lock, so the result may become stale in multitasking context.
*/
size_t NumOfMaxAllocableBytes(void){
	size_t X=0;
	T_BlkHeader *M=FirstFreeBlk;
	while(M){
		if(M->BlkLen>X)
			X=M->BlkLen;
		M=M->Next;
	}
	if(X)
		#if MALLOC_GUARD
			X-=(BLOCK_ALIGN+1);
		#else
			X-=BLOCK_ALIGN;
		#endif
	return X;
}

/*					HeapStatus
	Purpose:
		Checks heap consistency and reports heap diagnostic counters.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This routine first checks the free block chain, then walks the heap as a sequence of adjacent blocks. Heap
		access is protected while each diagnostic pass is performed.
	Input:
		HeapDimension: output pointer for the total heap block dimension counted during diagnostics.
		MaxBlockDimension: output pointer for the largest free block dimension.
		NumOfFreeBlock: output pointer for the number of free blocks.
		NumOfAllocatedBlock: output pointer for the number of allocated blocks.
	Output:
		HeapOk when the heap is consistent, otherwise a T_HeapStatus error code.
	Notes:
		When BLOCK_COUNTER is enabled, a mismatch between the live counter and the diagnostic count returns
		BlockNumberError.
*/
T_HeapStatus HeapStatus(T_Len *HeapDimension, T_Len *MaxBlockDimension, int *NumOfFreeBlock, int *NumOfAllocatedBlock){
	*HeapDimension=*MaxBlockDimension=0;
	*NumOfFreeBlock=*NumOfAllocatedBlock=0;
	T_BlkHeader *Cnt;
	// Primo step: verifica che i free block siano linkati a senso
	MM_START_PROTECTION;
	Cnt=FirstFreeBlk;
	while(Cnt!=NULL){
		(*NumOfFreeBlock)++;
		*HeapDimension+=Cnt->BlkLen;
		if(*MaxBlockDimension<Cnt->BlkLen)
			*MaxBlockDimension=Cnt->BlkLen;
		if((((DWORD)Cnt)%BLOCK_ALIGN)!=0){
			MM_END_PROTECTION;
			return BlkAddressError;
		}
		if(BYTE_PTR(Cnt)<HeapStart){
			MM_END_PROTECTION;
			return BlkOutOfHeapMemory;
		}
		if(BYTE_PTR(Cnt)+Cnt->BlkLen>HeapStart+HP_LEN){
			MM_END_PROTECTION;
			return BlkSizeError;
		}
		if((Cnt->BlkLen%BLOCK_ALIGN)!=0){
			MM_END_PROTECTION;
			return BlkSizeError;
		}
		if(Cnt->Next){
			if(BYTE_PTR(Cnt->Next)==BYTE_PTR(Cnt)+Cnt->BlkLen){
				MM_END_PROTECTION;
				return BlocchiLiberiAdiacenti;
			}
			if(BYTE_PTR(Cnt->Next)<BYTE_PTR(Cnt)+Cnt->BlkLen){
				MM_END_PROTECTION;
				return BlocchiLiberiSovrapposti;
			}
		}
		Cnt=Cnt->Next;
	}
	MM_END_PROTECTION;
	// Secondo step: verifica che considerando i blocchi come adiacenti il tutto
	//               abbia senso.
	MM_RESTART_PROTECTION;
	Cnt=FirstHeapBlk;
	do{
		(*NumOfAllocatedBlock)++;
		if(BYTE_PTR(Cnt)<HeapStart){
			MM_END_PROTECTION;
			return BlkOutOfHeapMemory;
		}
		if(BYTE_PTR(Cnt)>HeapStart+HP_LEN){
			MM_END_PROTECTION;
			return BlkSizeError;
		}
		if(Cnt->BlkLen==0){
			MM_END_PROTECTION;
			return BlkSizeError;
		}
		Cnt=(T_BlkHeader *)(BYTE_PTR(Cnt)+Cnt->BlkLen);
	}while(BYTE_PTR(Cnt)!=HeapStart+HP_LEN);
	MM_END_PROTECTION;
	*NumOfAllocatedBlock-=*NumOfFreeBlock;
	#if BLOCK_COUNTER
		if(BlockCounter!=*NumOfAllocatedBlock)
			return BlockNumberError;
	#endif
	return HeapOk;
}

/*					MM_Realloc
	Purpose:
		Resizes an allocated payload block by allocating a new block, copying data, and freeing the old block.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		This routine implements realloc() using malloc(), MM_UsableSize(), memcpy(), and free().
	Input:
		ptr: existing payload pointer, or NULL.
		new_size: requested new payload size in bytes.
	Output:
		Pointer to the resized payload, or NULL if allocation fails or new_size is zero.
	Notes:
		If ptr is NULL, this routine behaves like malloc(). If new_size is zero, it frees ptr and returns NULL.
*/
void* MM_Realloc(void* ptr, size_t new_size){
    if (ptr == NULL) return MM_Malloc(new_size);
    if (new_size == 0){
        MM_Free(ptr);
        return NULL;
    }
    size_t old_size = MM_UsableSize(ptr);  // vedi sotto
    void* new_ptr = MM_Malloc(new_size);
    if (!new_ptr) return NULL;
    size_t copy = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy);
    MM_Free(ptr);
    return new_ptr;
}

void* realloc(void* ptr, size_t new_size) __attribute__((alias("MM_Realloc")));
