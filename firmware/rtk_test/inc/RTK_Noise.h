#ifndef RTK_NOISE_H_
	#define RTK_NOISE_H_

	#include "CPP_Task.h"
	#include "RTK.h"

	#define NOISE_MEMORY_BLOCKS_PER_TASK 10U /* Max number of memory block allocated for every task */
	#define NOISE_MEMORY_STACK_WORDS 1024U
	#define NOISE_TASK_EXIT_TIMEOUT 1000U
	#define NOISE_MEMORY_MAX_BLOCK_BYTES 512U /* Max block dimension */


	class T_RTK_Noise: public T_CPP_Task{
		public:
			T_RTK_Noise();
			virtual ~T_RTK_Noise();
			static bool EndNoise();
		protected:
			static volatile Flag NoiseTerminateFlag;
		private:
			virtual void Noise(void)=0;
			virtual void Task(void);
			static Semaphore Sem;
			static volatile WORD NoiseInstanceCount;
	};


	class T_TaskMemoryCheck : public T_RTK_Noise {
		public:
			T_TaskMemoryCheck(BYTE TaskIdPar);
			virtual ~T_TaskMemoryCheck();
			static void ResetTestState(void);
			static volatile Flag FailureFlag;
			static volatile DWORD FailureCode;

		private:
			virtual void Noise(void);
			BYTE TaskId;
			DWORD Iteration;
			BYTE *Blocks[NOISE_MEMORY_BLOCKS_PER_TASK];
			WORD BlockSizes[NOISE_MEMORY_BLOCKS_PER_TASK];
			DWORD BlockStamps[NOISE_MEMORY_BLOCKS_PER_TASK];

			bool AllocateBlock(BYTE Slot);
			bool VerifyBlock(BYTE Slot);
			bool VerifyAllBlocks(void);
			void FillBlock(BYTE Slot);
			void FreeBlock(BYTE Slot);
			void FreeAllBlocks(void);
			void ReportFailure(DWORD Code);
	};

	class T_FloatNoise : public T_RTK_Noise {
		public:
			T_FloatNoise(BYTE TaskIdPar);
			virtual ~T_FloatNoise();
			static void ResetTestState(void);
			static volatile Flag FailureFlag;
			static volatile DWORD FailureCode;

		private:
			virtual void Noise(void);
			BYTE TaskId;
			DWORD Iteration;

			void BuildPattern(float *Values) const;
			bool VerifyPattern(const float *Expected, const float *Actual) const;
			void ReportFailure(DWORD Code) const;
	};

	#define NUM_TIMERS 10
	#define MAX_TIMER_VALUE 100

	class T_TaskTO_Check : public T_RTK_Noise {
		public:
			T_TaskTO_Check();
			virtual ~T_TaskTO_Check();
			virtual void Noise(void);
			void PrintCntTimerStatus(void);
		private:
			BYTE TaskId;
			DWORD Iteration;
			T_Timer Timer[NUM_TIMERS];

	};

	class T_TaskRandomSched: public T_RTK_Noise{
		public:
			T_TaskRandomSched();
			virtual ~T_TaskRandomSched();
			virtual void Noise(void);
	};

#endif
