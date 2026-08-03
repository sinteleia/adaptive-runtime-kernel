#include "main.h"
#include "RTK_TestBoard.h"
#include "Sched.h"
#include "Com.h"
#include "QueByte.h"
#include "general.h"
#include <stdio.h>
#include "RTK.h"

extern UART_HandleTypeDef huart2;

static TByteQue *RTK_TestConsoleRx;
static TByteQue *RTK_TestConsoleTx;
static bool *RTK_TestConsoleError;

typedef struct {
	GPIO_TypeDef *port;
	uint16_t pin;
} RTK_TestGpio;

static const RTK_TestGpio TestOut[5] = {
		{CN9_PIN1_GPIO_Port, CN9_PIN1_Pin},
		{CN9_PIN2_GPIO_Port, CN9_PIN2_Pin},
		{CN9_PIN3_GPIO_Port, CN9_PIN3_Pin},
		{CN9_PIN4_GPIO_Port, CN9_PIN4_Pin},
		{LD2_GPIO_Port, LD2_Pin}
};

static const RTK_TestGpio TestIn[1] = {
	{B1_GPIO_Port, B1_Pin}
};

int __io_putchar(int ch) {
	uint8_t c = (uint8_t)ch;
	if(RTK_TestConsoleTx==NULL) {
		HAL_UART_Transmit(&huart2, &c, 1U, HAL_MAX_DELAY);
	} else {
		while(!QuePut(RTK_TestConsoleTx, c)) {
			ActiveTx_1();
		}
		ActiveTx_1();
	}
	return ch;
}

/*
					RTK_TestBoardConsoleInit
	da controllare

		Inizializza la console USART2 del firmware di test usando le code byte
		di MyLib e le routine interrupt-driven della board.
*/
bool RTK_TestBoardConsoleInit(void) {
	RTK_TestConsoleRx=NewQue(128U);
	RTK_TestConsoleTx=NewQue(1024U);
	if((RTK_TestConsoleRx==NULL) || (RTK_TestConsoleTx==NULL)) {
		return false;
	}
	RTK_TestConsoleError=InitCom(1U, 115200U, 'N', 8U, 1U, RTK_TestConsoleRx, RTK_TestConsoleTx,
	                             5U, false, false);
	return RTK_TestConsoleError!=NULL;
}

bool IsConsolOutputEnded(void) {
	if(RTK_TestConsoleTx) return IS_QUE_EMPTY(RTK_TestConsoleTx->BinaryLenQueHeader.QueHeader);
	else return true;
}


void RTK_TestBoardInit(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	for (unsigned i = 0; i < 5U; i++) {
		HAL_GPIO_WritePin(TestOut[i].port, TestOut[i].pin, GPIO_PIN_RESET);
		GPIO_InitStruct.Pin = TestOut[i].pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(TestOut[i].port, &GPIO_InitStruct);
	}

	for (unsigned i = 0; i < 1U; i++) {
		GPIO_InitStruct.Pin = TestIn[i].pin;
		GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(TestIn[i].port, &GPIO_InitStruct);
	}

	__HAL_RCC_TIM2_CLK_ENABLE();
	__HAL_RCC_TIM1_CLK_ENABLE();

	TIM1->CR1=TIM_CR1_OPM | TIM_CR1_URS;
	TIM1->PSC=83U;
	TIM1->ARR=0U;
	TIM1->CNT=0U;
	TIM1->DIER=0U;
	TIM1->SR=0U;
	TIM1->EGR=TIM_EGR_UG;
	TIM1->SR=0U;
	NVIC_ClearPendingIRQ(TIM1_UP_TIM10_IRQn);
	NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0U);
	NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
	RTK_TestBoardOut(3U, false);

	TIM2->CR1=TIM_CR1_URS;
	TIM2->PSC=83U;
	TIM2->ARR=1998U;
	TIM2->CNT=0U;
	TIM2->DIER=0U;
	TIM2->SR=0U;
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn, 0U);
	NVIC_EnableIRQ(TIM2_IRQn);
}

void RTK_TestBoardWrite(const char *message) {
	while(*message!='\0') {
		(void)__io_putchar((unsigned char)*message);
		message++;
	}
}

/*
					GetCh
	da controllare

		Attende e legge un carattere dalla coda RX della console, usando l'RTK.
*/
char GetCh(void) {
	CheckAndWaitForAlmenoUnDWordBit((DWORD *)RTK_TestConsoleRx, 0xFFFFFFFF); CheckAndWaitForQueGet((TQueHeader *)RTK_TestConsoleRx);
	return LO(QueGet(RTK_TestConsoleRx));
}

void RTK_TestBoardOut(unsigned id, bool value) {
	if (id < 5U) HAL_GPIO_WritePin(TestOut[id].port, TestOut[id].pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool RTK_TestBoardIn(unsigned id) {
	if (id >= 1U) return false;
	return HAL_GPIO_ReadPin(TestIn[id].port, TestIn[id].pin) == GPIO_PIN_SET;
}

void RTK_TestLedSet(unsigned id, bool on) {
	if (id < 1U) RTK_TestBoardOut(id + 4U, on);
}

void RTK_TestLedToggle(unsigned id) {
	if (id < 1U) HAL_GPIO_TogglePin(TestOut[id + 4U].port, TestOut[id + 4U].pin);
}

DWORD RTK_TestTimestamp(void) {
	return HAL_GetTick();
}

Func TmrF;
/*
					RTK_TestAsyncTimerStart

		Attiva il timer asincrono usato per sollecitare attività di test, e memorizza l'indirizzo dewlla funzione da chiamare
	all'interno della relativa ISR.
*/
void RTK_TestAsyncTimerStart(Func F, unsigned int Us) {
	TmrF=F;
	RTK_TestBoardOut(2U, false);
	TIM2->CR1=TIM_CR1_URS;
	TIM2->DIER=0U;
	TIM2->SR=0U;
	TIM2->ARR=(Us>0U) ? (Us - 1U) : 0U;
	TIM2->EGR=TIM_EGR_UG;
	TIM2->SR=0U;
	TIM2->CNT=0U;
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	TIM2->DIER=TIM_DIER_UIE;
	TIM2->CR1=TIM_CR1_URS | TIM_CR1_CEN;
}

/*
					RTK_TestAsyncTimerStop

		Ferma il timer asincrono usato per sollecitare attività di test.
*/
void RTK_TestAsyncTimerStop(void) {
	TIM2->CR1&=~TIM_CR1_CEN;
	TIM2->DIER&=~TIM_DIER_UIE;
	TIM2->SR=0U;
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	RTK_TestBoardOut(2U, false);
}

Flag TimerDelayElapsed;

/*
					RTK_TestTimerDelay

	Purpose:
		Arm the delayed scheduling timer used by random scheduler tests.
	Author:
		Paolo Rozzi
	Reviewer:
		---
	Context:
		Task context.
	Input:
		uS: delay in microseconds. Values lower than 1 are treated as 1 us; values above the 16-bit timer range are clamped.
	Output:
		TimerDelayElapsed is cleared before the timer is started.
	Notes:
		TIM1 is configured with a 1 MHz counter clock and one-pulse mode.
*/
int dl_enter;
int dl_exit;
void RTK_TestTimerDelay(int uS) {
	dl_enter++;
	uint32_t delay;

	delay=(uS>1) ? (uint32_t)uS : 2U;
	if(delay>65536U) delay=65536U;

	TimerDelayElapsed=false;
	RTK_TestBoardOut(3U, true);
	TIM1->CR1=TIM_CR1_URS;
	TIM1->DIER=0U;
	TIM1->SR=0U;
	TIM1->PSC=83U;
	TIM1->ARR=delay - 1U;
	TIM1->CNT=0U;
	TIM1->EGR=TIM_EGR_UG;
	TIM1->SR=0U;
	TIM1->CNT=0U;
	NVIC_ClearPendingIRQ(TIM1_UP_TIM10_IRQn);
	NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
	TIM1->DIER=TIM_DIER_UIE;
	TIM1->CR1=TIM_CR1_OPM | TIM_CR1_URS | TIM_CR1_CEN;
}

void TIM1_UP_TIM10_IRQHandler(void) {
	if((TIM1->SR & TIM_SR_UIF)!=0U) {
		TIM1->SR&=~TIM_SR_UIF;
		TIM1->DIER&=~TIM_DIER_UIE;
		RTK_TestBoardOut(3U, false);
		TimerDelayElapsed=true;
		dl_exit++;
		SCHEDULE;
	}
	else
		while(1);
}

/*
					TIM2_IRQHandler

		ISR per il timer asincrono usato per sollecitare attività di test. Oltre ad attivare e disattivare l'uscita usata per
	monitorare l'attività di questo interrupt, lancia la funzione il cui indirizzo é memorizzato in TmrF.
*/
void TIM2_IRQHandler(void) {
	if((TIM2->SR & TIM_SR_UIF)!=0U) {
		RTK_TestBoardOut(2U, true);
		TIM2->SR&=~TIM_SR_UIF;
		if(KernelRunning) {
			TmrF();
			// SCHEDULE;
		}
		RTK_TestBoardOut(2U, false);
	}
}
