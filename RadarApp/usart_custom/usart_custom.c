/*H**********************************************************************
 * FILENAME :        usart_custom.c
 *
 * DESCRIPTION :
 *        Handling uarts transmit/receive operations by DMA.
 *
 * PUBLIC FUNCTIONS :
 *       void     UartCustomInit(UART_Custom_HandleTypeDef *,  UART_HandleTypeDef *)
 *       void     void uartCustomReceiveDMA(UART_Custom_HandleTypeDef *huartCustom)
 *       void     uartCustomTransmitDMA(UART_Custom_HandleTypeDef *huartCustom, uint8_t msgLen)
 *
 * AUTHOR :    Kamil Kośnik        START DATE :    18 July 2022
 *
 *H*/
#include "usart.h"
#include <stm32g0xx_hal_dma.h>
#include "usart_custom.h"

//extern const uint8_t softVersion[3];////////////////////////////////version of software////////////////////////////////////
extern UART_Custom_HandleTypeDef huartPC;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern UART_Queue uartQueuePC;

//Initialization of custom uart struct instance
//@param  huartCustom - instance of custom uart struct
//@param  huart - instance of HAL uart UART_handleTypeDef struct
//@param  dataRxMaxLen - maximum length of received data by uart DMA
//@param  dataTxMaxLen - maximum length of transmitted data by uart DMA
void uartCustomInit(UART_Custom_HandleTypeDef *huartCustom, UART_HandleTypeDef *huart, uint16_t rxDataLen, uint16_t txDataLen)
{
	huartCustom->huart = huart;
	huartCustom->txTransmissionCmplt = true;
	huartCustom->txTransmissionCmpltTime = 0;
	huartCustom->dataRxMaxLen = rxDataLen;
	huartCustom->dataTxMaxLen = txDataLen;
}

//Receive data through uart dma until idle state or maximum bytes received
//@param  huartCustom - instance of custom uart struct
inline HAL_StatusTypeDef uartCustomReceiveDMA(UART_Custom_HandleTypeDef *huartCustom)
{
	__HAL_UART_SEND_REQ(huartCustom->huart, UART_RXDATA_FLUSH_REQUEST);
	return HAL_UARTEx_ReceiveToIdle_DMA(huartCustom->huart, (uint8_t*) huartCustom->dataRx, huartCustom->dataRxMaxLen);
}

//Transmit data through uart dma
//@param  huartCustom - instance of custom uart struct
//@param  msgLen - amount of bytes to be sent
inline HAL_StatusTypeDef uartCustomTransmitDMA(UART_Custom_HandleTypeDef *huartCustom, uint16_t msgLen)
{
	if(huartCustom == &huartPC)
	{
		uartQueueAdd(&uartQueuePC, huartCustom->dataTx, msgLen);
		return HAL_OK;
	}
	return HAL_BUSY;
}

inline void uartQueueReceive(volatile UART_Queue *uartQueue)
{
	if(uartQueue->tailIndex != uartQueue->headIxndex && uartQueue->dataRx[uartQueue->tailIndex][0] != 0)
	{
		switch(uartQueue->dataRx[uartQueue->tailIndex][0])
		{

		case 0:
			break;

		default:
			break;
		}

		uartQueue->dataRx[uartQueue->tailIndex][0] = 0;
		uartQueue->dataRxLength[uartQueue->tailIndex] = 0;

		++uartQueue->tailIndex;
		if(uartQueue->tailIndex == UART_QUEUE_LEN)
		{
			uartQueue->tailIndex = 0;
		}
	}
}

void uartQueueInit(volatile UART_Queue *uartQueue)
{
	uartQueue->dataIndex = 0;
	uartQueue->transmitIndex = 0;

	uartQueue->headIxndex = 0;
	uartQueue->tailIndex = 0;
}

inline void uartQueueAdd(volatile UART_Queue *uartQueue, uint8_t *dataTxSource, uint16_t msgLen)
{
	memcpy((uint8_t *)uartQueue->dataTx[uartQueue->dataIndex], dataTxSource, msgLen);
	uartQueue->dataTxLength[uartQueue->dataIndex] = msgLen + 1;
	uartQueue->dataIndex = (++uartQueue->dataIndex) % UART_QUEUE_LEN;
}

inline void uartQueueClear(volatile UART_Queue *uartQueue)
{
	for(uint8_t i = 0; i < UART_QUEUE_LEN; ++i)
	{
		uartQueue->dataTx[i][0] = 0;
	}

	uartQueue->transmitIndex = 0;
	uartQueue->dataIndex = 0;
}

inline void uartQueueTransmit(volatile UART_Queue *uartQueue, UART_Custom_HandleTypeDef *huartCustom)
{
	if(uartQueue == &uartQueuePC)
	{
		if(huartCustom->txTransmissionCmplt && (HAL_GetTick() - huartCustom->txTransmissionCmpltTime) > UART_TX_TRANSMISSION_CMPLT_TIMEOUT)
		{
			if(uartQueue->dataTx[uartQueue->transmitIndex][0] != 0)
			{
				huartCustom->txTransmissionCmplt = false;
				if(uartQueue->dataTxLength[uartQueue->transmitIndex] > huartCustom->dataTxMaxLen)
				{
					uartQueue->dataTxLength[uartQueue->transmitIndex] = huartCustom->dataTxMaxLen;
				}
				HAL_UART_StateTypeDef result = HAL_UART_Transmit_DMA(huartCustom->huart,(uint8_t *) uartQueue->dataTx[uartQueue->transmitIndex], uartQueue->dataTxLength[uartQueue->transmitIndex]);
				__HAL_DMA_DISABLE_IT(&hdma_usart1_tx, DMA_IT_HT);
			}
		}
	}
}

//////////////////////Interrupt callbacks///////////////////////////////

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart->Instance == huartPC.huart->Instance)
	{
		memcpy((void *)uartQueuePC.dataRx[uartQueuePC.headIxndex], (void *)huartPC.dataRx, Size);
		uartQueuePC.dataRxLength[uartQueuePC.headIxndex] = Size;

		++uartQueuePC.headIxndex;
		if(uartQueuePC.headIxndex == UART_QUEUE_LEN)
		{
			uartQueuePC.headIxndex = 0;
		}

		uartCustomReceiveDMA(&huartPC);
		__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == huartPC.huart->Instance)
	{
		huartPC.txTransmissionCmplt = true;
		huartPC.txTransmissionCmpltTime = HAL_GetTick();
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == huartPC.huart->Instance)
	{
		__HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
		huart->ErrorCode = 0;
		HAL_DMA_Abort(huart->hdmarx); 			// Disable the DMA channel
		HAL_UART_Receive_DMA(huart, NULL, 0); 	// Flush the UART RX buffer
		HAL_DMA_Init(huart->hdmarx);			// Reconfigure the DMA channel
		uartCustomReceiveDMA(&huartPC);			// Restart the DMA transfer
		__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
	}
}
