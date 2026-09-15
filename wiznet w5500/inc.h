/*
 * w5500.h
 *
 *  Created on: 15-Sept-2026
 *      Author: Identium
 */

#ifndef INC_W5500_H_
#define INC_W5500_H_

#include "stm32f429xx.h"
#include "stm32f429xx_driver_spi.h"

#define W5500_VERSIONR    0x0039U
#define W5500_VERSION     0x04U

void W5500_Init(void);
uint8_t W5500_ReadVersion(void);


uint8_t W5500_ReadRegisters(
    uint16_t address,
    uint8_t blockSelect,
    uint8_t *buffer,
    uint16_t length
);

uint8_t W5500_WriteRegisters(
    uint16_t address,
    uint8_t blockSelect,
    const uint8_t *buffer,
    uint16_t length
);

uint8_t W5500_ReadVersion(void);

#endif /* INC_W5500_H_ */
