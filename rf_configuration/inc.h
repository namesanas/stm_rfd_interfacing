/*
 * config.h
 *
 *  Created on: 12-Sept-2026
 *      Author: Identium
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

#include "stm32f429xx.h"
#include "stm32f429xx_driver_gpio.h"
#include "stm32f429xx_driver_uart.h"
#include "stm32f429xx_driver_spi.h"
#include "sillion_application.h"

/*
 * ============================================================
 * GLOBAL PERIPHERAL HANDLES
 * ============================================================
 */

extern USART_Handle_t usart3;
extern USART_Handle_t usart1;

extern Silion_Handle_t silion;
extern SILION_ReaderConfig_t readerConfig;

/*
 * ============================================================
 * HARDWARE / PERIPHERAL CONFIGURATION
 * ============================================================
 */

void RF_Configuration_Init(void);

extern volatile uint8_t txComplete;
extern volatile uint8_t rxComplete;

void SILION_ClearRxQueue(void);
void SILION_ClearUartFlags(void);


/*
 * ============================================================
 * W5500 HARDWARE CONFIGURATION
 * ============================================================
 */

#define W5500_CS_GPIO_PORT       GPIOB
#define W5500_CS_PIN             GPIO_PIN_NO_12

#define W5500_RST_GPIO_PORT      GPIOC
#define W5500_RST_PIN            GPIO_PIN_NO_0

extern SPI_Handle_t w5500Spi;


#endif /* INC_CONFIG_H_ */
