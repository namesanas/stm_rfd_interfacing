/*
 * stm32f429xx_rcc_driver.h
 *
 * Created on: Jul 16, 2026
 * Author: anas
 */

#ifndef INC_STM32F429XX_RCC_DRIVER_H_
#define INC_STM32F429XX_RCC_DRIVER_H_

#include "stm32f429xx.h"


/*
 * Returns APB1 peripheral clock value
 */
uint32_t RCC_GetPCLK1Value(void);


/*
 * Returns APB2 peripheral clock value
 */
uint32_t RCC_GetPCLK2Value(void);


/*
 * Returns system clock value
 */
uint32_t RCC_GetSystemClockValue(void);


/*
 * Returns PLL output clock value
 */
uint32_t RCC_GetPLLOutputClock(void);


#endif /* INC_STM32F429XX_RCC_DRIVER_H_ */
