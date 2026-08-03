#include "RTK_Noise.h"
#include "RTK_TestBoard.h"
#include "RTK_TestDiag.h"
#include "MM.H"
#include "stdio.h"
#include "Microdelay.h"

#include <string.h>

bool T_RTK_Noise::EndNoise(){
	NoiseTerminateFlag=true;
	if(!WaitForNessunWordBitTO(&NoiseInstanceCount, 0xFFFF, NOISE_TASK_EXIT_TIMEOUT))
		return false;
	WaitForTime(100);	// Attende per essere certi che le task siano state eliminate
	NoiseTerminateFlag=false;
	return true;
}

volatile Flag T_RTK_Noise::NoiseTerminateFlag=false;
volatile WORD T_RTK_Noise::NoiseInstanceCount=0;


T_RTK_Noise::T_RTK_Noise(){
	RTK_ExclusiveIncrement(NoiseInstanceCount);
}

T_RTK_Noise::~T_RTK_Noise(){
	RTK_ExclusiveDecrement(NoiseInstanceCount);
}

void T_RTK_Noise::Task(void){
	char S[13];
	snprintf(S, sizeof(S), "NOISE TSK %2u", (unsigned int)(NoiseInstanceCount % 100U));
	SetTaskName(S);
	Noise();
}

volatile Flag T_TaskMemoryCheck::FailureFlag;
volatile DWORD T_TaskMemoryCheck::FailureCode;

#define FLOAT_NOISE_REG_COUNT 16U
#define FLOAT_NOISE_ROUNDS    8U

volatile Flag T_FloatNoise::FailureFlag;
volatile DWORD T_FloatNoise::FailureCode;

/*
					T_TaskMemoryCheck

	Purpose:
		Initialize one C++ task instance used to stress the RTK heap allocator with randomized allocation activity.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Created by RTK test code when background memory noise is required.
	Input:
		TaskIdPar - Instance identifier used to differentiate check patterns and failure codes.
	Output:
		The object fields are initialized and the shared instance counter is incremented.
*/
T_TaskMemoryCheck::T_TaskMemoryCheck(BYTE TaskIdPar):T_RTK_Noise() {
	TaskId=TaskIdPar;
	Iteration=0;
	memset(Blocks, 0, sizeof(Blocks));
	memset(BlockSizes, 0, sizeof(BlockSizes));
	memset(BlockStamps, 0, sizeof(BlockStamps));
}

/*
					~T_TaskMemoryCheck

	Purpose:
		Register that one memory noise task object has been destroyed.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Executed by CPP_TaskExec after Task() returns and the C++ task object is deleted.
	Input:
		None.
	Output:
		The shared instance counter is decremented when it is non-zero.
*/
T_TaskMemoryCheck::~T_TaskMemoryCheck() {
}

/*
					ResetTestState

	Purpose:
		Restore the shared memory noise state before creating a new group of worker tasks.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by RTK tests before starting memory noise workers.
	Input:
		None.
	Output:
		Shared termination, failure, and instance counters are reset.
*/
void T_TaskMemoryCheck::ResetTestState(void) {
	FailureFlag=false;
	FailureCode=0;
}

/*
					Task

	Purpose:
		Continuously verify, allocate, rewrite, and free heap blocks until the shared termination flag is set.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Executed as a background RTK C++ task during memory and concurrency tests.
	Input:
		None.
	Output:
		Allocated blocks are released before the task returns; FailureFlag and FailureCode are set on errors.
*/
void T_TaskMemoryCheck::Noise(void){
	while(!NoiseTerminateFlag && !FailureFlag) {
		if(!VerifyAllBlocks()) break;
		BYTE Slot=(BYTE)(RTK_TestRandom() % NOISE_MEMORY_BLOCKS_PER_TASK);
		if(Blocks[Slot]==NULL){
			if(!AllocateBlock(Slot)) break;
		}
		else if((RTK_TestRandom() & 0x03U)!=0U) FreeBlock(Slot);
		else{
			BlockStamps[Slot]=RTK_TestRandom() ^ Iteration ^ ((DWORD)TaskId << 16) ^ ((DWORD)Slot << 8);
			FillBlock(Slot);
		}
		Iteration++;
	}
	VerifyAllBlocks();
	FreeAllBlocks();
}

/*
					AllocateBlock

	Purpose:
		Allocate one heap block in the selected slot and initialize it with a verifiable pattern.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by T_TaskMemoryCheck::Task when a randomly selected slot is empty.
	Input:
		Slot - Slot index to allocate.
	Output:
		true when allocation and verification succeed; false when allocation or verification fails.
*/
bool T_TaskMemoryCheck::AllocateBlock(BYTE Slot) {
	WORD Size=(WORD)(1U + (RTK_TestRandom() % NOISE_MEMORY_MAX_BLOCK_BYTES));
	BYTE *Block=(BYTE *)malloc(Size);
	if(Block==NULL){
		ReportFailure(0x1000UL | Slot);
		return false;
	}
	Blocks[Slot]=Block;
	BlockSizes[Slot]=Size;
	BlockStamps[Slot]=RTK_TestRandom() ^ Iteration ^ ((DWORD)TaskId << 16) ^ ((DWORD)Slot << 8);
	FillBlock(Slot);
	return VerifyBlock(Slot);
}

/*
					VerifyBlock

	Purpose:
		Check that one allocated block still contains the expected pattern and has a usable size compatible with the request.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by memory noise workers before rewriting or freeing a block.
	Input:
		Slot - Slot index to verify.
	Output:
		true when the slot is empty or the block contents are valid; false when a mismatch is detected.
*/
bool T_TaskMemoryCheck::VerifyBlock(BYTE Slot) {
	if(Blocks[Slot]==NULL) return true;
	if(MM_UsableSize(Blocks[Slot])<BlockSizes[Slot]) {
		ReportFailure(0x2000UL | Slot);
		return false;
	}
	for(WORD Index=0; Index<BlockSizes[Slot]; Index++) {
		BYTE Expected=(BYTE)(BlockStamps[Slot] + Index + (Index >> 3) + TaskId);
		if(Blocks[Slot][Index]!=Expected) {
			ReportFailure(0x3000UL | Slot);
			return false;
		}
	}
	return true;
}

/*
					VerifyAllBlocks

	Purpose:
		Verify every live block owned by one memory noise worker.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called repeatedly by T_TaskMemoryCheck::Task to catch heap corruption produced by concurrent activity.
	Input:
		None.
	Output:
		true when all blocks are valid; false after the first detected mismatch.
*/
bool T_TaskMemoryCheck::VerifyAllBlocks(void) {
	for(BYTE Slot=0; Slot<NOISE_MEMORY_BLOCKS_PER_TASK; Slot++) {
		if(!VerifyBlock(Slot)) return false;
	}
	return true;
}

/*
					FillBlock

	Purpose:
		Fill one allocated block with a deterministic pattern derived from its stamp, offset, and task instance.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called after allocating a block or changing its stamp.
	Input:
		Slot - Slot index to fill.
	Output:
		The block payload is overwritten with the expected pattern.
*/
void T_TaskMemoryCheck::FillBlock(BYTE Slot) {
	for(WORD Index=0; Index<BlockSizes[Slot]; Index++) {
		Blocks[Slot][Index]=(BYTE)(BlockStamps[Slot] + Index + (Index >> 3) + TaskId);
	}
}

/*
					FreeBlock

	Purpose:
		Verify and release the heap block stored in one slot.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by memory noise workers when a random choice selects a live block for release.
	Input:
		Slot - Slot index to free.
	Output:
		The slot metadata is cleared after the block is released or after verification fails.
*/
void T_TaskMemoryCheck::FreeBlock(BYTE Slot) {
	if(VerifyBlock(Slot)) {
		free(Blocks[Slot]);
	}
	Blocks[Slot]=NULL;
	BlockSizes[Slot]=0;
	BlockStamps[Slot]=0;
}

/*
					FreeAllBlocks

	Purpose:
		Release all heap blocks still owned by one memory noise worker.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called before T_TaskMemoryCheck::Task returns.
	Input:
		None.
	Output:
		All live slots are released.
*/
void T_TaskMemoryCheck::FreeAllBlocks(void) {
	for(BYTE Slot=0; Slot<NOISE_MEMORY_BLOCKS_PER_TASK; Slot++) {
		if(Blocks[Slot]!=NULL) FreeBlock(Slot);
	}
}

/*
					ReportFailure

	Purpose:
		Store the first failure detected by any memory noise worker.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called after allocation or verification errors.
	Input:
		Code - Failure code containing the error class and slot index.
	Output:
		FailureFlag and FailureCode are updated if no previous failure was recorded.
*/
void T_TaskMemoryCheck::ReportFailure(DWORD Code) {
	uint32_t SchedulerLock=RTK_SchedulerLock();
	if(!FailureFlag) {
		FailureFlag=true;
		FailureCode=Code;
	}
	RTK_Unlock(SchedulerLock);
}

/*
					T_FloatNoise

	Purpose:
		Initialize one floating-point noise task instance with a seed that differentiates its register pattern.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Created by RTK test code when floating-point context switch stress is required.
	Input:
		TaskIdPar - Instance identifier used to derive the expected register pattern and failure codes.
	Output:
		The task object is initialized.
	Notes:
		The first implementation stresses S16-S31, which are the floating-point registers normally preserved by software.
*/
T_FloatNoise::T_FloatNoise(BYTE TaskIdPar):T_RTK_Noise() {
	TaskId=TaskIdPar;
	Iteration=0;
}

/*
					~T_FloatNoise

	Purpose:
		Destroy one floating-point noise task object.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Executed by CPP_TaskExec after Task() returns and the C++ task object is deleted.
	Input:
		None.
	Output:
		None.
*/
T_FloatNoise::~T_FloatNoise() {
}

/*
					ResetTestState

	Purpose:
		Restore the shared floating-point noise state before creating a new group of worker tasks.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by RTK tests before starting floating-point noise workers.
	Input:
		None.
	Output:
		Shared failure state is reset.
*/
void T_FloatNoise::ResetTestState(void) {
	FailureFlag=false;
	FailureCode=0;
}

/*
					Noise

	Purpose:
		Repeatedly load S16-S31 with a deterministic pattern, perform reversible floating-point operations, schedule away,
		and verify that the register contents survived context switches.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Executed as a background RTK C++ task during floating-point context switch tests.
	Input:
		None.
	Output:
		FailureFlag and FailureCode are set if any register value is corrupted.
	Notes:
		The multiply-by-two and multiply-by-half operations are exactly reversible for the generated normal values.
*/
void T_FloatNoise::Noise(void) {
	float Expected[FLOAT_NOISE_REG_COUNT];
	float Actual[FLOAT_NOISE_REG_COUNT];
	const float Two=2.0f;
	const float Half=0.5f;

	while(!NoiseTerminateFlag && !FailureFlag) {
		BuildPattern(Expected);

		__asm volatile(
			"vldmia %[expected], {s16-s31}\n"
			:
			: [expected] "r" (Expected)
			: "memory", "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
			  "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31");

		for(BYTE Round=0; Round<FLOAT_NOISE_ROUNDS; Round++) {
			__asm volatile(
				"vldr s0, %[two]\n"
				"vldr s1, %[half]\n"
				"vmul.f32 s16, s16, s0\n"
				"vmul.f32 s17, s17, s0\n"
				"vmul.f32 s18, s18, s0\n"
				"vmul.f32 s19, s19, s0\n"
				"vmul.f32 s20, s20, s0\n"
				"vmul.f32 s21, s21, s0\n"
				"vmul.f32 s22, s22, s0\n"
				"vmul.f32 s23, s23, s0\n"
				"vmul.f32 s24, s24, s0\n"
				"vmul.f32 s25, s25, s0\n"
				"vmul.f32 s26, s26, s0\n"
				"vmul.f32 s27, s27, s0\n"
				"vmul.f32 s28, s28, s0\n"
				"vmul.f32 s29, s29, s0\n"
				"vmul.f32 s30, s30, s0\n"
				"vmul.f32 s31, s31, s0\n"
				"vmul.f32 s16, s16, s1\n"
				"vmul.f32 s17, s17, s1\n"
				"vmul.f32 s18, s18, s1\n"
				"vmul.f32 s19, s19, s1\n"
				"vmul.f32 s20, s20, s1\n"
				"vmul.f32 s21, s21, s1\n"
				"vmul.f32 s22, s22, s1\n"
				"vmul.f32 s23, s23, s1\n"
				"vmul.f32 s24, s24, s1\n"
				"vmul.f32 s25, s25, s1\n"
				"vmul.f32 s26, s26, s1\n"
				"vmul.f32 s27, s27, s1\n"
				"vmul.f32 s28, s28, s1\n"
				"vmul.f32 s29, s29, s1\n"
				"vmul.f32 s30, s30, s1\n"
				"vmul.f32 s31, s31, s1\n"
				:
				: [two] "m" (Two), [half] "m" (Half)
				: "memory", "s0", "s1", "s16", "s17", "s18", "s19", "s20", "s21",
				  "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31");
			SCHEDULE;
		}

		__asm volatile(
			"vstmia %[actual], {s16-s31}\n"
			:
			: [actual] "r" (Actual)
			: "memory");

		if(!VerifyPattern(Expected, Actual)) break;
		Iteration++;
	}
}

/*
					BuildPattern

	Purpose:
		Generate the expected floating-point register pattern for the current task instance and iteration.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by T_FloatNoise::Noise before loading S16-S31.
	Input:
		Values - Destination array with FLOAT_NOISE_REG_COUNT entries.
	Output:
		Values is filled with finite normal single-precision numbers.
*/
void T_FloatNoise::BuildPattern(float *Values) const {
	for(BYTE Index=0; Index<FLOAT_NOISE_REG_COUNT; Index++) {
		Values[Index]=(float)(TaskId + 1U) +
		              ((float)(Index + 1U) * (1.0f / 32.0f)) +
		              ((float)(Iteration & 0x0FU) * (1.0f / 1024.0f));
	}
}

/*
					VerifyPattern

	Purpose:
		Compare the expected and actual floating-point register patterns.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called by T_FloatNoise::Noise after S16-S31 have been stored back to memory.
	Input:
		Expected - Expected register values.
		Actual - Values read back from the floating-point registers.
	Output:
		true when all values match exactly; false after recording the first mismatch.
*/
bool T_FloatNoise::VerifyPattern(const float *Expected, const float *Actual) const {
	for(BYTE Index=0; Index<FLOAT_NOISE_REG_COUNT; Index++) {
		if(Actual[Index]!=Expected[Index]) {
			ReportFailure(0x4000UL | ((DWORD)TaskId << 8) | Index);
			return false;
		}
	}
	return true;
}

/*
					ReportFailure

	Purpose:
		Store the first failure detected by any floating-point noise worker.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Called after a floating-point register verification mismatch.
	Input:
		Code - Failure code containing the error class, task id, and register index.
	Output:
		FailureFlag and FailureCode are updated if no previous failure was recorded.
*/
void T_FloatNoise::ReportFailure(DWORD Code) const {
	uint32_t SchedulerLock=RTK_SchedulerLock();
	if(!FailureFlag) {
		FailureFlag=true;
		FailureCode=Code;
	}
	RTK_Unlock(SchedulerLock);
}

T_TaskTO_Check::T_TaskTO_Check(){
	for(unsigned int i=0; i<NUM_TIMERS; i++) InitTimer(&Timer[i]);
}

T_TaskTO_Check::~T_TaskTO_Check(){
	for(unsigned int i=0; i<NUM_TIMERS; i++) DisarmaTimer(&Timer[i]);
}

void T_TaskTO_Check::Noise(){
	T_TO TO=PresetTO(RTK_TestRandom()%35);
	while(!NoiseTerminateFlag){
		MicroDelay(RTK_TestRandom()%100);
		int n=RTK_TestRandom()%NUM_TIMERS;
		SetTimer(&Timer[n], RTK_TestRandom()%MAX_TIMER_VALUE);
		n=RTK_TestRandom()%NUM_TIMERS;
		DisarmaTimer(&Timer[n]);
		if(IsToElapsed(TO)){
			WaitForTime(RTK_TestRandom()%35);
			TO=PresetTO(RTK_TestRandom()%35);
		}
	}
}

void T_TaskTO_Check::PrintCntTimerStatus(void){
	uint32_t l=RTK_SysTicLock();
	for(int n=0; n<NUM_TIMERS; n++){
		if(IS_TIMER_ELAPSED(Timer[n])) printf("--- ");
		else printf("%03i ", (int)TimerTicQuantoManca(&Timer[n]));
	}
	RTK_Unlock(l);
	printf("\n\r");
}

T_TaskRandomSched::T_TaskRandomSched(){
}

T_TaskRandomSched::~T_TaskRandomSched(){
}

void T_TaskRandomSched::Noise(){
	while(!NoiseTerminateFlag){
		RTK_TestTimerDelay(RTK_TestRandom()%500);
		WaitForFlag(&TimerDelayElapsed);
	}
}


