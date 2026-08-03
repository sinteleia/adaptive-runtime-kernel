#ifndef __COM_H
	#define __COM_H

	/*			DRIVERS scheda SINED
		Dobbiamo fornire il supporto per:
			UART4 (seriale di servizio scheda WIFI);
			USART2 (Seriale isolata A);
			USART3 (Seriale isolata B);
			SPI1 (SPI chip seconda ethernet);
			SPI2 (SPI WIFI);
	*/

	#include "main.h"
	#include "Type.h"
	#include "Librerie.h"
	#include "MyIntrinsics.h"

	/*		Le seriali di questi micro sono di 3 tipi, UART, USART ed LPUART. Non ho guardato l'LPUART ma, se usate in asincrono,
		UART ed USART dovrebbero essere equivalenti per cui il SW � uno solo per entrambe.
			N-B: Per motivi di compatibilit� con il software scritto per il Fujitsu le com sono numerate da 0 a NUM_COMS-1
		(penso dipendente dal micro) e corrispondono alle UART/USART da 1 a NUM_COMS. Sempre per motivi di compatibilit� con il
		SW precedente ker_ck, Priority, UseRTS ed UseCTS hanno dei default e possono essere omesse.
			Per usare le com occorre:
				1)	Programmare il mux del clock delle seriali utilizzate. Il micro ha tre mux, uno usato per USART 1 e 6, uno
					usato per LPUART1 ed uno usato per tutte le altre.
				2)	Programmare i pin usati dalla seriale per l'alternate function corretta;
				3)	Creare le FIFO per TX ed RX di dimensioni adeguate;
				4)	Invocare la InitCom passando il puntatore alle FIFO di Tx e di RX e specificando se si usano i segnali di
					controllo o meno. Se uno dei due puntatori vale NULL la COM viene inizializzata in sola ricezione o in sola
					trasmissione,  se entrambi valgono NULL la COM viene disabilitata. Si devono passare alla init anche il valore
					del clock a monte del prescaler e la priorit� da assegnare all'interrupt.
			Quando abbiamo tempo di farlo possiamo anche vedere di gestire le FIFO, per ora omettiamo.
	*/

	#ifdef __cplusplus
		extern "C"{
	#endif

	#ifdef __cplusplus
		bool *InitCom(BYTE ComN, DWORD BPS, char Parity, BYTE Len, BYTE Stop, TByteQue *Rx, TByteQue *Tx,
		              BYTE Priority=5, bool UseCTS=false, bool UseRTS=false);
	#else
		bool *InitCom(BYTE ComN, DWORD BPS, char Parity, BYTE Len, BYTE Stop, TByteQue *Rx, TByteQue *Tx,
		              BYTE Priority, bool UseCTS, bool UseRTS);
	#endif
	void FreeCom(BYTE ComN);
	void ActiveTx(BYTE ComN);

	static inline void ActiveTx_0(void){START_PROTECTION; USART1->CR1|=USART_CR1_TXEIE;  END_PROTECTION;}
	static inline void ActiveTx_1(void){START_PROTECTION; USART2->CR1|=USART_CR1_TXEIE; END_PROTECTION;}
	static inline void ActiveTx_2(void){START_PROTECTION; USART3->CR1|=USART_CR1_TXEIE; END_PROTECTION;}
	static inline void ActiveTx_3(void){START_PROTECTION; UART4->CR1|=USART_CR1_TXEIE; END_PROTECTION;}
	static inline void ActiveTx_4(void){START_PROTECTION; UART5->CR1|=USART_CR1_TXEIE; END_PROTECTION;}
	static inline void ActiveTx_5(void){START_PROTECTION; USART6->CR1|=USART_CR1_TXEIE; END_PROTECTION;}

	void USART1_IRQHandler(void);
	void USART2_IRQHandler(void);
	void USART3_IRQHandler(void);
	void UART4_IRQHandler(void);
	void UART5_IRQHandler(void);
	void USART6_IRQHandler(void);

	#ifdef __cplusplus
		}
	#endif

#endif
