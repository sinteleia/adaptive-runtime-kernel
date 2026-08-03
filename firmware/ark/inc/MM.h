/*						ARK Project - Adaptive Runtime Kernel

	Module:
		MM.h

	Purpose:
		Public interface for the ARK memory manager.

	Description:
		This header declares heap initialization, allocation, release and diagnostic services exposed by
		the ARK memory manager. It also selects the heap length type from MM.cfg and exposes optional
		allocation counters when block counting diagnostics are enabled.

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
#ifndef __MM_H
	#define __MM_H

	#include "Type.h"
	#include "MM.cfg"

	#if BIG_HEAP
		typedef DWORD T_Len;
	#else
		typedef WORD T_Len;
	#endif

	#if BLOCK_COUNTER
		extern DWORD TotalAllocExecuted;
		extern short int BlockCounter;
	#endif

	#ifdef __cplusplus
		extern "C" {
	#endif

	/*							minit
		Purpose:
			Initialize the default linker-defined heap.
		Context:
			Call once before using any memory allocation service that relies on the default heap.
		Input:
			None.
		Output:
			None.
	*/
	void minit(void);
	
	/*							MinitFromBlock
		Purpose:
			Initialize the heap from a caller-provided memory block.
		Context:
			Use instead of minit() when the heap is not defined by linker symbols.
		Input:
			Ad - Start address of the memory block to use as the heap.
			l - Size of the memory block in bytes.
		Output:
			None.
	*/
	void MinitFromBlock(void *Ad, T_Len l);

	/*							malloc
		Purpose:
			Allocate a payload block from the ARK heap.
		Context:
			Available after the selected heap initialization routine has completed.
		Input:
			Sz - Requested payload size in bytes.
		Output:
			Pointer to the allocated payload, or NULL when Sz is zero or no suitable block is available.
	*/
	void *malloc(size_t Sz);

	/*							calloc
		Purpose:
			Allocate an array from the ARK heap and initialize its payload to zero.
		Context:
			Available after the selected heap initialization routine has completed.
		Input:
			number - Number of array elements.
			size - Size of each array element in bytes.
		Output:
			Pointer to the zero-filled payload, or NULL when allocation fails or the total size overflows.
	*/
	void *calloc(size_t number, size_t size);

	/*							realloc
		Purpose:
			Resize an allocated payload while preserving as much existing data as possible.
		Context:
			Available after the selected heap initialization routine has completed.
		Input:
			ptr - Existing payload pointer, or NULL to perform a new allocation.
			size - Requested new payload size in bytes; zero releases ptr.
		Output:
			Pointer to the resized payload, or NULL when allocation fails or size is zero.
		Notes:
			On allocation failure, the original block remains allocated and unchanged.
	*/
	void *realloc(void *ptr, size_t size);

	/*							free
		Purpose:
			Return a previously allocated payload block to the ARK heap.
		Context:
			Use only with NULL or a live pointer returned by an ARK allocation service.
		Input:
			Blk - Payload pointer to release; NULL has no effect.
		Output:
			None.
	*/
	void free(void *Blk);
  
	/*							MM_UsableSize
		Purpose:
			Return the usable payload size of an allocated heap block.
		Context:
			Use with NULL or a live pointer returned by an ARK allocation service.
		Input:
			Blk - Payload pointer to inspect.
		Output:
			Usable payload size in bytes, or zero when Blk is NULL.
	*/
	T_Len MM_UsableSize(void *Blk);

	typedef enum{
		HeapOk,
		BlkAddressError,
		BlkOutOfHeapMemory,
		BlkSizeError,
		BlocchiLiberiAdiacenti,
		BlocchiLiberiSovrapposti,
		BlockNumberError,
		GuardiaSfondata
	}T_HeapStatus;
 
	/*							HeapStatus
		Purpose:
			Check free-list and physical-block consistency and collect heap diagnostic values.
		Context:
			Use after heap initialization when a consistency snapshot is required.
		Input:
			HeapDimension - Output pointer for the total heap dimension found during the scan.
			MaxBlockDimension - Output pointer for the largest free-block dimension.
			NumOfFreeBlock - Output pointer for the number of free blocks.
			NumOfAllocatedBlock - Output pointer for the number of allocated blocks.
		Output:
			HeapOk when the heap is consistent, otherwise the detected T_HeapStatus error.
	*/
	T_HeapStatus HeapStatus(T_Len *HeapDimension, T_Len *MaxBlockDimension, int *NumOfFreeBlock, int *NumOfAllocatedBlock);

	/*							NumOfMaxAllocableBytes
		Purpose:
			Return the largest payload size that can currently be allocated from the heap.
		Context:
			Use after heap initialization for informational or diagnostic purposes.
		Input:
			None.
		Output:
			Largest currently allocable payload size in bytes.
		Notes:
			The result is a snapshot and may become stale if another execution context changes the heap.
	*/
	size_t NumOfMaxAllocableBytes(void);

	#ifdef __cplusplus
		}
	#endif
	
	#define FREE(x) free(x); x=NULL;
	
#endif
