/configuaration structure for USARTx peripheral

/*
 * Configuration structure for USARTx peripheral
 */
typedef struct
{
	uint8_t USART_Mode;
	uint32_t USART_Baud;
	uint8_t USART_NoOfStopBits;
	uint8_t USART_WordLength;
	uint8_t USART_ParityControl;
	uint8_t USART_HWFlowControl;
	uint8_t USART_OverSampling;
}USART_Config_t;


/*
 * Handle structure for USARTx peripheral
 */
typedef struct
{
	USART_RegDef_t *pUSARTx;
	USART_Config_t   USART_Config;
	uint8_t 		*pTxBuffer;			//to store Tx buffer address
	uint8_t 		*pRxBuffer;
	uint32_t 		TxLen;				//to store Tx len
	uint32_t 		RxLen;
	uint8_t			TxBusyState;
	uint8_t			RxBusyState;
}USART_Handle_t;

/*
 * @USART_MODE
 */
#define USART_MODE_ONLY_TX        0
#define USART_MODE_ONLY_RX        1
#define USART_MODE_TXRX           2


/*
 * @USART_BAUD
 */
#define USART_STD_BAUD_1200       1200
#define USART_STD_BAUD_2400       2400
#define USART_STD_BAUD_9600       9600
#define USART_STD_BAUD_19200      19200
#define USART_STD_BAUD_38400      38400
#define USART_STD_BAUD_57600      57600
#define USART_STD_BAUD_115200     115200
#define USART_STD_BAUD_230400     230400
#define USART_STD_BAUD_460800     460800
#define USART_STD_BAUD_921600     921600
#define USART_STD_BAUD_2M         2000000
#define USART_STD_BAUD_3M         3000000


/*
 * @USART_PARITY
 */
#define USART_PARITY_DISABLE      0
#define USART_PARITY_EN_EVEN      1
#define USART_PARITY_EN_ODD       2


/*
 * @USART_WORD_LENGTH
 */
#define USART_WORDLEN_8BITS       0
#define USART_WORDLEN_9BITS       1


/*
 * @USART_STOP_BITS
 */
#define USART_STOPBITS_1          0
#define USART_STOPBITS_0_5        1
#define USART_STOPBITS_2          2
#define USART_STOPBITS_1_5        3


/*
 * @USART_HW_FLOW_CONTROL
 */
#define USART_HW_FLOW_CTRL_NONE   0
#define USART_HW_FLOW_CTRL_CTS    1
#define USART_HW_FLOW_CTRL_RTS    2
#define USART_HW_FLOW_CTRL_CTSRTS 3


/*
 * @USART_BAUD_OVERSAMPLING
 */
#define USART_OVERSAMPLING_16     0
#define USART_OVERSAMPLING_8      1


/*
 * @USART_FLAG
 */
#define USART_FLAG_TXE            USART_SR_TXE
#define USART_FLAG_TC             USART_SR_TC
#define USART_FLAG_RXNE           USART_SR_RXNE
#define USART_FLAG_IDLE           USART_SR_IDLE
#define USART_FLAG_ORE            USART_SR_ORE
#define USART_FLAG_FE             USART_SR_FE
#define USART_FLAG_NE             USART_SR_NE
#define USART_FLAG_PE             USART_SR_PE


/*
 * @USART_IRQ
 */
#define USART_IRQ_TXE             USART_CR1_TXEIE
#define USART_IRQ_TC              USART_CR1_TCIE
#define USART_IRQ_RXNE            USART_CR1_RXNEIE
#define USART_IRQ_IDLE            USART_CR1_IDLEIE
#define USART_IRQ_PE              USART_CR1_PEIE


/*
 * @USART_IRQ_EVENTS
 */
#define USART_EVENT_TX_CMPLT      0
#define USART_EVENT_RX_CMPLT      1
#define USART_EVENT_IDLE          2
#define USART_EVENT_CTS           3

/* Peripheral clock control */
void USART_PeriClockControl(USART_RegDef_t *pUSARTx,uint8_t EnorDi);


/* USART initialization/de-initialization */
void USART_Init(USART_Handle_t *pUSARTHandle);
void USART_DeInit(USART_RegDef_t *pUSARTx);


/* Data transmission */
void USART_SendData(USART_Handle_t *pUSARTHandle,uint8_t *pTxBuffer,uint32_t Len);
void USART_ReceiveData(USART_Handle_t *pUSARTHandle,uint8_t *pRxBuffer,uint32_t Len);


/* Flag status */
uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx,uint8_t StatusFlagName);


/* Interrupt-based transmission */
uint8_t USART_SendDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pTxBuffer,uint32_t Len);
uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pRxBuffer,uint32_t Len);


/* IRQ configuration */
void USART_IRQInterruptConfig(uint8_t IRQNumber,uint8_t EnorDi);
void USART_IRQPriorityConfig(uint8_t IRQNumber,uint32_t IRQPriority);


/* IRQ handling */
void USART_IRQHandling(USART_Handle_t *pUSARTHandle);


/* Application callback */
void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle,uint8_t AppEvent);


/* Baud-rate configuration */
void USART_SetBaudRate(USART_RegDef_t *pUSARTx,uint32_t BaudRate);
