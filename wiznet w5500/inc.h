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

#define W5500_GAR      0x0001U
#define W5500_SUBR     0x0005U
#define W5500_SHAR     0x0009U
#define W5500_SIPR     0x000FU

#define W5500_S0_RXBUF_SIZE    0x001EU
#define W5500_S0_TXBUF_SIZE    0x001FU

#define W5500_BSB_SOCKET0      1U

#define W5500_S0_MR       0x0000U
#define W5500_S0_CR       0x0001U
#define W5500_S0_SR       0x0003U
#define W5500_S0_PORT     0x0004U

#define W5500_S0_BSB      1U

#define W5500_Sn_CR_OPEN  0x01U
#define W5500_Sn_CR_LISTEN 0x02U

#define W5500_Sn_SR_INIT   0x13U
#define W5500_Sn_SR_LISTEN 0x14U

#define W5500_S0_IR  0x0002U

#define W5500_S0_RX_RSR      0x0026U
#define W5500_S0_RX_RD       0x0028U
#define W5500_S0_RX_WR       0x002AU
#define W5500_S0_CR          0x0001U

#define W5500_S0_RECV        0x40U

#define W5500_S0_TX_FSR      0x0020U
#define W5500_S0_TX_WR       0x0024U

#define W5500_S0_SEND        0x20U

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

void W5500_SetNetworkConfig(void);

uint8_t W5500_ReadVersion(void);

uint8_t W5500_StartTCPServer(uint16_t port);

uint16_t W5500_Socket0_GetRxSize(void);


uint16_t W5500_Socket0_Receive(
    uint8_t *buffer,
    uint16_t bufferSize
);

uint16_t W5500_Socket0_GetTxFreeSize(void);

uint16_t W5500_Socket0_Send(
    const uint8_t *buffer,
    uint16_t length
);




#endif /* INC_W5500_H_ */
