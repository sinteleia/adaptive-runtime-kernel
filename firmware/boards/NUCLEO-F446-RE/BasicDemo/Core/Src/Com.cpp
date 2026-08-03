#include "Com.h"

#define NUM_COMS 6

USART_TypeDef * const USARTS[]{USART1, USART2, USART3, UART4, UART5, USART6};
const IRQn_Type USART_VECTORS[]{USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn, USART6_IRQn};
TByteQue *RxQues[NUM_COMS];
TByteQue *TxQues[NUM_COMS];
bool RxError[NUM_COMS];

bool *InitCom(BYTE ComN, DWORD BPS, char Parity, BYTE Len, BYTE Stop, TByteQue *Rx, TByteQue *Tx, BYTE Priority,
              bool UseCTS, bool UseRTS){
	if(ComN>=NUM_COMS) return NULL;
	NVIC_DisableIRQ(USART_VECTORS[ComN]);
	RxQues[ComN]=Rx;
	TxQues[ComN]=Tx;
	if((Rx==NULL)&&(Tx==NULL)){
		FreeCom(ComN);
		return NULL;
	}
	switch(ComN+1){
		case 1: {START_PROTECTION; RCC->APB2ENR|=RCC_APB2ENR_USART1EN; END_PROTECTION; break;}
		case 2: {START_PROTECTION; RCC->APB1ENR|=RCC_APB1ENR_USART2EN; END_PROTECTION; break;}
		case 3: {START_PROTECTION; RCC->APB1ENR|=RCC_APB1ENR_USART3EN; END_PROTECTION; break;}
		case 4: {START_PROTECTION; RCC->APB1ENR|=RCC_APB1ENR_UART4EN; END_PROTECTION; break;}
		case 5: {START_PROTECTION; RCC->APB1ENR|=RCC_APB1ENR_UART5EN; END_PROTECTION; break;}
		case 6: {START_PROTECTION; RCC->APB2ENR|=RCC_APB2ENR_USART6EN; END_PROTECTION; break;}
	}
	USART_TypeDef *USART=USARTS[ComN];
	USART->CR1=0;
	DWORD Tmp=0;
	switch(Len){
		case 8: break;
		case 9: Tmp|=USART_CR1_M; break;
		default: return NULL;
	}
	switch(Parity){
		case 'o': case 'O': Tmp|=USART_CR1_PCE|USART_CR1_PS|USART_CR1_PEIE; break;
		case 'E': case 'e': Tmp|=USART_CR1_PCE|USART_CR1_PEIE; break;
	}
	if(Rx!=NULL) Tmp|=USART_CR1_RE|USART_CR1_RXNEIE;
	if(Tx!=NULL) Tmp|=USART_CR1_TE;
	USART->CR1=Tmp;
	USART->CR2=Stop==2? USART_CR2_STOP_1: 0;
	Tmp=USART_CR3_EIE;
	if(UseRTS) Tmp|=USART_CR3_RTSE;
	if(UseCTS) Tmp|=USART_CR3_CTSE;
	USART->CR3=Tmp;
	DWORD ker_ck=((ComN==0U) || (ComN==5U)) ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq();
	Tmp=(ker_ck + (BPS / 2U)) / BPS;
	if((Tmp==0U) || (Tmp>0xFFFFU)) return NULL;
	USART->BRR=Tmp;
	USART->CR1|=USART_CR1_UE;
	NVIC_EnableIRQ(USART_VECTORS[ComN]);
	NVIC_SetPriority(USART_VECTORS[ComN], Priority);
	return &RxError[ComN];
}

void FreeCom(BYTE ComN){
	if(ComN>=NUM_COMS) return;
	NVIC_DisableIRQ(USART_VECTORS[ComN]);
	USARTS[ComN]->CR1=0;
	switch(ComN+1){
		case 1: {START_PROTECTION; RCC->APB2ENR&=~RCC_APB2ENR_USART1EN; END_PROTECTION; break;}
		case 2: {START_PROTECTION; RCC->APB1ENR&=~RCC_APB1ENR_USART2EN; END_PROTECTION; break;}
		case 3: {START_PROTECTION; RCC->APB1ENR&=~RCC_APB1ENR_USART3EN; END_PROTECTION; break;}
		case 4: {START_PROTECTION; RCC->APB1ENR&=~RCC_APB1ENR_UART4EN; END_PROTECTION; break;}
		case 5: {START_PROTECTION; RCC->APB1ENR&=~RCC_APB1ENR_UART5EN; END_PROTECTION; break;}
		case 6: {START_PROTECTION; RCC->APB2ENR&=~RCC_APB2ENR_USART6EN; END_PROTECTION; break;}
	}
}

void ActiveTx(BYTE ComN){
	if(ComN>=NUM_COMS) return;
	START_PROTECTION
	USARTS[ComN]->CR1|=USART_CR1_TXEIE;
	END_PROTECTION;
}

__attribute__((used)) void USART1_IRQHandler(void){
	WORD Tmp;

	if(USART1->SR&(USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)){
		RxError[0]=true;
		Tmp=(WORD)USART1->SR;
		Tmp=(WORD)USART1->DR;
		(void)Tmp;
	}
	while(USART1->SR&USART_SR_RXNE){
		if(!QuePut(RxQues[0], USART1->DR&USART_DR_DR)) RxError[0]=true;
	}
	while(USART1->SR&USART_SR_TXE){
		Tmp=QueGet(TxQues[0]);
		if(Tmp) USART1->DR=Tmp&USART_DR_DR;
		else{
			USART1->CR1&=~USART_CR1_TXEIE;
			return;
		}
	}
}

__attribute__((used)) void USART2_IRQHandler(void){
	WORD Tmp;

	if(USART2->SR&(USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)){
		RxError[1]=true;
		Tmp=(WORD)USART2->SR;
		Tmp=(WORD)USART2->DR;
		(void)Tmp;
	}
	while(USART2->SR&USART_SR_RXNE){
		if(!QuePut(RxQues[1], USART2->DR&USART_DR_DR)) RxError[1]=true;
	}
	while(USART2->SR&USART_SR_TXE){
		Tmp=QueGet(TxQues[1]);
		if(Tmp) USART2->DR=Tmp&USART_DR_DR;
		else{
			USART2->CR1&=~USART_CR1_TXEIE;
			return;
		}
	}
}

__attribute__((used)) void USART3_IRQHandler(void){
	WORD Tmp;

	if(USART3->SR&(USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)){
		RxError[2]=true;
		Tmp=(WORD)USART3->SR;
		Tmp=(WORD)USART3->DR;
		(void)Tmp;
	}
	while(USART3->SR&USART_SR_RXNE){
		if(!QuePut(RxQues[2], USART3->DR&USART_DR_DR)) RxError[2]=true;
	}
	while(USART3->SR&USART_SR_TXE){
		Tmp=QueGet(TxQues[2]);
		if(Tmp) USART3->DR=Tmp&USART_DR_DR;
		else{
			USART3->CR1&=~USART_CR1_TXEIE;
			return;
		}
	}
}

__attribute__((used)) void UART4_IRQHandler(void){
	WORD Tmp;

	if(UART4->SR&(USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)){
		RxError[3]=true;
		Tmp=(WORD)UART4->SR;
		Tmp=(WORD)UART4->DR;
		(void)Tmp;
	}
	while(UART4->SR&USART_SR_RXNE){
		if(!QuePut(RxQues[3], UART4->DR&USART_DR_DR)) RxError[3]=true;
	}
	while(UART4->SR&USART_SR_TXE){
		Tmp=QueGet(TxQues[3]);
		if(Tmp) UART4->DR=Tmp&USART_DR_DR;
		else{
			UART4->CR1&=~USART_CR1_TXEIE;
			return;
		}
	}
}

__attribute__((used)) void UART5_IRQHandler(void){
	WORD Tmp;

	if(UART5->SR&(USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)){
		RxError[4]=true;
		Tmp=(WORD)UART5->SR;
		Tmp=(WORD)UART5->DR;
		(void)Tmp;
	}
	while(UART5->SR&USART_SR_RXNE){
		if(!QuePut(RxQues[4], UART5->DR&USART_DR_DR)) RxError[4]=true;
	}
	while(UART5->SR&USART_SR_TXE){
		Tmp=QueGet(TxQues[4]);
		if(Tmp) UART5->DR=Tmp&USART_DR_DR;
		else{
			UART5->CR1&=~USART_CR1_TXEIE;
			return;
		}
	}
}

__attribute__((used)) void USART6_IRQHandler(void){
	WORD Tmp;

	if(USART6->SR&(USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)){
		RxError[5]=true;
		Tmp=(WORD)USART6->SR;
		Tmp=(WORD)USART6->DR;
		(void)Tmp;
	}
	while(USART6->SR&USART_SR_RXNE){
		if(!QuePut(RxQues[5], USART6->DR&USART_DR_DR)) RxError[5]=true;
	}
	while(USART6->SR&USART_SR_TXE){
		Tmp=QueGet(TxQues[5]);
		if(Tmp) USART6->DR=Tmp&USART_DR_DR;
		else{
			USART6->CR1&=~USART_CR1_TXEIE;
			return;
		}
	}
}
