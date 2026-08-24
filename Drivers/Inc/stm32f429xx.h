/*
 * stm32f429xx..h
 *
 *  Created on: 21-Aug-2026
 *      Author: Identium
 */

#ifndef INC_STM32F429XX_H_
#define INC_STM32F429XX_H_

/*
 * stm32f429xx.h
 *
 *  Created on: 20-Aug-2026
 *      Author: Identium
 */

//1. Includes / header guard



#include <stdint.h>
#include <stddef.h>

//2. Processor-specific macros

#define NVIC_ISER0          ((__vo uint32_t*)0xE000E100)
#define NVIC_ISER1          ((__vo uint32_t*)0xE000E104)
#define NVIC_ISER2          ((__vo uint32_t*)0xE000E108)
#define NVIC_ISER3          ((__vo uint32_t*)0xE000E10C)

#define NVIC_ICER0          ((__vo uint32_t*)0xE000E180)
#define NVIC_ICER1          ((__vo uint32_t*)0xE000E184)
#define NVIC_ICER2          ((__vo uint32_t*)0xE000E188)
#define NVIC_ICER3          ((__vo uint32_t*)0xE000E18C)

#define NVIC_PR_BASE_ADDR   ((__vo uint32_t*)0xE000E400)

#define NO_PR_BITS_IMPLEMENTED  4

#define NVIC_IRQ_PRI0       0
#define NVIC_IRQ_PRI1       1
#define NVIC_IRQ_PRI2       2
#define NVIC_IRQ_PRI3       3
#define NVIC_IRQ_PRI4       4
#define NVIC_IRQ_PRI5       5
#define NVIC_IRQ_PRI6       6
#define NVIC_IRQ_PRI7       7
#define NVIC_IRQ_PRI8       8
#define NVIC_IRQ_PRI9       9
#define NVIC_IRQ_PRI10      10
#define NVIC_IRQ_PRI11      11
#define NVIC_IRQ_PRI12      12
#define NVIC_IRQ_PRI13      13
#define NVIC_IRQ_PRI14      14
#define NVIC_IRQ_PRI15      15

#define IRQ_NO_EXTI0				6
#define IRQ_NO_EXTI1				7
#define IRQ_NO_EXTI2				8
#define IRQ_NO_EXTI3				9
#define IRQ_NO_EXTI4				10
#define IRQ_NO_EXTI9_5				23
#define IRQ_NO_EXTI15_10			40

#define IRQ_NO_SPI1					35
#define IRQ_NO_SPI2					36
#define IRQ_NO_SPI3					51

#define IRQ_NO_I2C_EV				31
#define IRQ_NO_I2C_ER				32


//3. Memory map

#define FLASH_BASEADDR      0x08000000U
#define SRAM1_BASEADDR      0x20000000U

/*
 * STM32F429ZI has 256 KB SRAM.
 * We will refine the SRAM layout later if we need SRAM2/CCM details.
 */

#define SRAM1_SIZE         (256U * 1024U)
#define SRAM2_BASEADDR     (SRAM1_BASEADDR + SRAM1_SIZE)
#define ROM                0x1FFF0000U
#define SRAM               SRAM1_BASEADDR
#define __vo               volatile


//4. Peripheral base addresses

#define PERIPH_BASE        0x40000000U

#define APB1PERIPH_BASEADDR    PERIPH_BASE
#define APB2PERIPH_BASEADDR    0x40010000U

#define AHB1PERIPH_BASEADDR    0x40020000U
#define AHB2PERIPH_BASEADDR    0x50000000U

#define RCC_BASEADDR       (AHB1PERIPH_BASEADDR + 0x3800U)



//5. GPIO register definitions

//gpio peripheral definition

typedef struct
{
    __vo uint32_t MODER;
    __vo uint32_t OTYPER;
    __vo uint32_t OSPEEDR;
    __vo uint32_t PUPDR;
    __vo uint32_t IDR;
    __vo uint32_t ODR;
    __vo uint32_t BSRR;
    __vo uint32_t LCKR;
    __vo uint32_t AFR[2];

} GPIO_RegDef_t;

//gpio base address

#define GPIOA_BASEADDR       (AHB1PERIPH_BASEADDR + 0x0000U)
#define GPIOB_BASEADDR       (AHB1PERIPH_BASEADDR + 0x0400U)
#define GPIOC_BASEADDR       (AHB1PERIPH_BASEADDR + 0x0800U)
#define GPIOD_BASEADDR       (AHB1PERIPH_BASEADDR + 0x0C00U)
#define GPIOE_BASEADDR       (AHB1PERIPH_BASEADDR + 0x1000U)
#define GPIOF_BASEADDR       (AHB1PERIPH_BASEADDR + 0x1400U)
#define GPIOG_BASEADDR       (AHB1PERIPH_BASEADDR + 0x1800U)
#define GPIOH_BASEADDR       (AHB1PERIPH_BASEADDR + 0x1C00U)
#define GPIOI_BASEADDR       (AHB1PERIPH_BASEADDR + 0x2000U)
#define GPIOJ_BASEADDR       (AHB1PERIPH_BASEADDR + 0x2400U)
#define GPIOK_BASEADDR       (AHB1PERIPH_BASEADDR + 0x2800U)

//peripheral pointers

#define GPIOA       ((GPIO_RegDef_t*)GPIOA_BASEADDR)
#define GPIOB       ((GPIO_RegDef_t*)GPIOB_BASEADDR)
#define GPIOC       ((GPIO_RegDef_t*)GPIOC_BASEADDR)
#define GPIOD       ((GPIO_RegDef_t*)GPIOD_BASEADDR)
#define GPIOE       ((GPIO_RegDef_t*)GPIOE_BASEADDR)
#define GPIOF       ((GPIO_RegDef_t*)GPIOF_BASEADDR)
#define GPIOG       ((GPIO_RegDef_t*)GPIOG_BASEADDR)
#define GPIOH       ((GPIO_RegDef_t*)GPIOH_BASEADDR)
#define GPIOI       ((GPIO_RegDef_t*)GPIOI_BASEADDR)
#define GPIOJ       ((GPIO_RegDef_t*)GPIOJ_BASEADDR)
#define GPIOK       ((GPIO_RegDef_t*)GPIOK_BASEADDR)

//clock control macros

#define GPIOA_PCLK_EN()     (RCC->AHB1ENR |= (1 << 0))
#define GPIOB_PCLK_EN()     (RCC->AHB1ENR |= (1 << 1))
#define GPIOC_PCLK_EN()     (RCC->AHB1ENR |= (1 << 2))
#define GPIOD_PCLK_EN()     (RCC->AHB1ENR |= (1 << 3))
#define GPIOE_PCLK_EN()     (RCC->AHB1ENR |= (1 << 4))
#define GPIOF_PCLK_EN()     (RCC->AHB1ENR |= (1 << 5))
#define GPIOG_PCLK_EN()     (RCC->AHB1ENR |= (1 << 6))
#define GPIOH_PCLK_EN()     (RCC->AHB1ENR |= (1 << 7))
#define GPIOI_PCLK_EN()     (RCC->AHB1ENR |= (1 << 8))
#define GPIOJ_PCLK_EN()      (RCC->AHB1ENR |= (1 << 9))
#define GPIOK_PCLK_EN()      (RCC->AHB1ENR |= (1 << 10))

#define GPIOA_PCLK_DI()     (RCC->AHB1ENR &= ~(1 << 0))
#define GPIOB_PCLK_DI()     (RCC->AHB1ENR &= ~(1 << 1))
#define GPIOC_PCLK_DI()     (RCC->AHB1ENR &= ~(1 << 2))
#define GPIOD_PCLK_DI()     (RCC->AHB1ENR &= ~(1 << 3))
#define GPIOE_PCLK_DI()     (RCC->AHB1ENR &= ~(1 << 4))
#define GPIOF_PCLK_DI()     (RCC->AHB1ENR &= ~(1 << 5))
#define GPIOG_PCLK_DI()     (RCC->AHB1ENR &= ~(1 << 6))
#define GPIOH_PCLK_DI()     (RCC->AHB1ENR &= ~(1 << 7))
#define GPIOI_PCLK_DI()     (RCC->AHB1ENR &= ~(1 << 8))
#define GPIOJ_PCLK_DI()      (RCC->AHB1ENR &= ~(1 << 9))
#define GPIOK_PCLK_DI()      (RCC->AHB1ENR &= ~(1 << 10))

//gpio reset macros

#define GPIOA_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 0)); (RCC->AHB1RSTR &= ~(1 << 0)); } while(0)
#define GPIOB_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 1)); (RCC->AHB1RSTR &= ~(1 << 1)); } while(0)
#define GPIOC_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 2)); (RCC->AHB1RSTR &= ~(1 << 2)); } while(0)
#define GPIOD_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 3)); (RCC->AHB1RSTR &= ~(1 << 3)); } while(0)
#define GPIOE_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 4)); (RCC->AHB1RSTR &= ~(1 << 4)); } while(0)
#define GPIOF_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 5)); (RCC->AHB1RSTR &= ~(1 << 5)); } while(0)
#define GPIOG_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 6)); (RCC->AHB1RSTR &= ~(1 << 6)); } while(0)
#define GPIOH_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 7)); (RCC->AHB1RSTR &= ~(1 << 7)); } while(0)
#define GPIOI_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 8)); (RCC->AHB1RSTR &= ~(1 << 8)); } while(0)
#define GPIOJ_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 9)); (RCC->AHB1RSTR &= ~(1 << 9)); } while(0)
#define GPIOK_REG_RESET()   do { (RCC->AHB1RSTR |= (1 << 10)); (RCC->AHB1RSTR &= ~(1 << 10)); } while(0)

#define SYSCFG_PCLK_EN()    (RCC->APB2ENR |= (1 << 14))
#define SYSCFG_PCLK_DI()    (RCC->APB2ENR &= ~(1 << 14))

#define SYSCFG_REG_RESET() do { RCC->APB2RSTR |= (1 << 14); RCC->APB2RSTR &= ~(1 << 14); } while(0)

//6. SPI register definitions

//spi peripheral definition

typedef struct
{
    __vo uint32_t CR1;
    __vo uint32_t CR2;
    __vo uint32_t SR;
    __vo uint32_t DR;
    __vo uint32_t CRCPR;
    __vo uint32_t RXCRCR;
    __vo uint32_t TXCRCR;
    __vo uint32_t I2SCFGR;
    __vo uint32_t I2SPR;

} SPI_RegDef_t;

//spi base address

#define SPI1_BASEADDR       (APB2PERIPH_BASE + 0x3000U)
#define SPI2_BASEADDR       (APB1PERIPH_BASE + 0x3800U)
#define SPI3_BASEADDR       (APB1PERIPH_BASE + 0x3C00U)
#define SPI4_BASEADDR       (APB2PERIPH_BASE + 0x3400U)
#define SPI5_BASEADDR       (APB2PERIPH_BASE + 0x5000U)
#define SPI6_BASEADDR       (APB2PERIPH_BASE + 0x5400U)

//peripheral pointers


#define SPI1       ((SPI_RegDef_t*)SPI1_BASEADDR)
#define SPI2       ((SPI_RegDef_t*)SPI2_BASEADDR)
#define SPI3       ((SPI_RegDef_t*)SPI3_BASEADDR)
#define SPI4       ((SPI_RegDef_t*)SPI4_BASEADDR)
#define SPI5       ((SPI_RegDef_t*)SPI5_BASEADDR)
#define SPI6       ((SPI_RegDef_t*)SPI6_BASEADDR)

//clock control macros

/* APB2 */
#define SPI1_PCLK_EN()      (RCC->APB2ENR |= (1 << 12))
#define SPI4_PCLK_EN()      (RCC->APB2ENR |= (1 << 13))
#define SPI5_PCLK_EN()      (RCC->APB2ENR |= (1 << 20))
#define SPI6_PCLK_EN()      (RCC->APB2ENR |= (1 << 21))

/* APB1 */
#define SPI2_PCLK_EN()      (RCC->APB1ENR |= (1 << 14))
#define SPI3_PCLK_EN()      (RCC->APB1ENR |= (1 << 15))

#define SPI1_PCLK_DI()      (RCC->APB2ENR &= ~(1 << 12))
#define SPI4_PCLK_DI()      (RCC->APB2ENR &= ~(1 << 13))
#define SPI5_PCLK_DI()      (RCC->APB2ENR &= ~(1 << 20))
#define SPI6_PCLK_DI()      (RCC->APB2ENR &= ~(1 << 21))

#define SPI2_PCLK_DI()      (RCC->APB1ENR &= ~(1 << 14))
#define SPI3_PCLK_DI()      (RCC->APB1ENR &= ~(1 << 15))

#define SPI1_REG_RESET()    do {(RCC->APB2RSTR |= (1 << 12)); (RCC->APB2RSTR &= ~(1 << 12)); } while(0)
#define SPI4_REG_RESET()    do {(RCC->APB2RSTR |= (1 << 13)); (RCC->APB2RSTR &= ~(1 << 13)); } while(0)
#define SPI5_REG_RESET()    do {(RCC->APB2RSTR |= (1 << 20)); (RCC->APB2RSTR &= ~(1 << 20)); } while(0)
#define SPI6_REG_RESET()    do {(RCC->APB2RSTR |= (1 << 21)); (RCC->APB2RSTR &= ~(1 << 21)); } while(0)
#define SPI2_REG_RESET()    do {(RCC->APB1RSTR |= (1 << 14)); (RCC->APB1RSTR &= ~(1 << 14)); } while(0)
#define SPI3_REG_RESET()    do {(RCC->APB1RSTR |= (1 << 15)); (RCC->APB1RSTR &= ~(1 << 15)); } while(0)

//spi registers

#define SPI_CR1_CPHA       0
#define SPI_CR1_CPOL       1
#define SPI_CR1_MSTR       2
#define SPI_CR1_BR         3
#define SPI_CR1_SPE        6
#define SPI_CR1_LSBFIRST   7
#define SPI_CR1_SSI        8
#define SPI_CR1_SSM        9
#define SPI_CR1_RXONLY     10
#define SPI_CR1_DFF        11
#define SPI_CR1_CRCNEXT    12
#define SPI_CR1_CRCEN      13
#define SPI_CR1_BIDIOE     14
#define SPI_CR1_BIDIMODE   15

#define SPI_CR2_RXDMAEN    0
#define SPI_CR2_TXDMAEN    1
#define SPI_CR2_SSOE       2
#define SPI_CR2_FRF        4
#define SPI_CR2_ERRIE      5
#define SPI_CR2_RXNEIE     6
#define SPI_CR2_TXEIE      7

#define SPI_SR_CHSIDE      2
#define SPI_SR_UDR         3
#define SPI_SR_CRCERR      4
#define SPI_SR_MODF        5
#define SPI_SR_OVR         6
#define SPI_SR_BSY         7
#define SPI_SR_FRE         8
#define SPI_SR_FRLVL       9
#define SPI_SR_FTLVL       11
#define SPI_SR_TXE         1
#define SPI_SR_RXNE        0


//7. I2C register definitions

//i2c gpio

typedef struct
{
    __vo uint32_t CR1;
    __vo uint32_t CR2;
    __vo uint32_t OAR1;
    __vo uint32_t OAR2;
    __vo uint32_t DR;
    __vo uint32_t SR1;
    __vo uint32_t SR2;
    __vo uint32_t CCR;
    __vo uint32_t TRISE;
    __vo uint32_t FLTR;
} I2C_RegDef_t;

//spi base address

#define I2C1_BASEADDR      (APB1PERIPH_BASEADDR + 0x5400)
#define I2C2_BASEADDR      (APB1PERIPH_BASEADDR + 0x5800)
#define I2C3_BASEADDR      (APB1PERIPH_BASEADDR + 0x5C00)

//peripheral pointers

#define I2C1    ((I2C_RegDef_t*)I2C1_BASEADDR)
#define I2C2    ((I2C_RegDef_t*)I2C2_BASEADDR)
#define I2C3    ((I2C_RegDef_t*)I2C3_BASEADDR)

//clock control macros

#define I2C1_PCLK_EN()    (RCC->APB1ENR |= (1 << 21))
#define I2C2_PCLK_EN()    (RCC->APB1ENR |= (1 << 22))
#define I2C3_PCLK_EN()    (RCC->APB1ENR |= (1 << 23))

#define I2C1_PCLK_DI()    (RCC->APB1ENR &= ~(1 << 21))
#define I2C2_PCLK_DI()    (RCC->APB1ENR &= ~(1 << 22))
#define I2C3_PCLK_DI()    (RCC->APB1ENR &= ~(1 << 23))

#define I2C1_REG_RESET() do { (RCC->APB1RSTR |= (1 << 21)); (RCC->APB1RSTR &= ~(1 << 21)); } while(0)
#define I2C2_REG_RESET() do { (RCC->APB1RSTR |= (1 << 22)); (RCC->APB1RSTR &= ~(1 << 22)); } while(0)
#define I2C3_REG_RESET() do { (RCC->APB1RSTR |= (1 << 23)); (RCC->APB1RSTR &= ~(1 << 23)); } while(0)

//i2c registers

#define I2C_CR1_PE             0
#define I2C_CR1_START          8
#define I2C_CR1_STOP           9
#define I2C_CR1_ACK            10
#define I2C_CR1_POS            11
#define I2C_CR1_SWRST          15

#define I2C_CR2_FREQ           0

#define I2C_OAR1_ADD0          0
#define I2C_OAR1_ADD7          1
#define I2C_OAR1_ADD8          8
#define I2C_OAR1_ADD9          9
#define I2C_OAR1_ADDMODE       15

#define I2C_SR1_SB             0
#define I2C_SR1_ADDR           1
#define I2C_SR1_BTF            2
#define I2C_SR1_ADD10          3
#define I2C_SR1_STOPF          4
#define I2C_SR1_RXNE           6
#define I2C_SR1_TXE            7
#define I2C_SR1_BERR           8
#define I2C_SR1_ARLO           9
#define I2C_SR1_AF             10
#define I2C_SR1_OVR            11
#define I2C_SR1_PECERR         12
#define I2C_SR1_TIMEOUT        14
#define I2C_SR1_SMBALERT       15

#define I2C_SR2_MSL            0
#define I2C_SR2_BUSY           1
#define I2C_SR2_TRA            2
#define I2C_SR2_GENCALL        4
#define I2C_SR2_SMBDEFAULT     5
#define I2C_SR2_SMBHOST        6
#define I2C_SR2_DUALF          7




//8. USART register definitions

typedef struct
{
    __vo uint32_t SR;
    __vo uint32_t DR;
    __vo uint32_t BRR;
    __vo uint32_t CR1;
    __vo uint32_t CR2;
    __vo uint32_t CR3;
    __vo uint32_t GTPR;
} USART_RegDef_t;

#define USART1_BASEADDR      (APB2PERIPH_BASEADDR + 0x1000)
#define USART6_BASEADDR      (APB2PERIPH_BASEADDR + 0x4400)

#define USART2_BASEADDR      (APB1PERIPH_BASEADDR + 0x4400)
#define USART3_BASEADDR      (APB1PERIPH_BASEADDR + 0x4800)
#define UART4_BASEADDR       (APB1PERIPH_BASEADDR + 0x4C00)
#define UART5_BASEADDR       (APB1PERIPH_BASEADDR + 0x5000)
#define UART7_BASEADDR       (APB1PERIPH_BASEADDR + 0x7800)
#define UART8_BASEADDR       (APB1PERIPH_BASEADDR + 0x7C00)

//peripheral pointers

#define USART1    ((USART_RegDef_t*)USART1_BASEADDR)
#define USART2    ((USART_RegDef_t*)USART2_BASEADDR)
#define USART3    ((USART_RegDef_t*)USART3_BASEADDR)
#define UART4     ((USART_RegDef_t*)UART4_BASEADDR)
#define UART5     ((USART_RegDef_t*)UART5_BASEADDR)
#define USART6    ((USART_RegDef_t*)USART6_BASEADDR)
#define UART7     ((USART_RegDef_t*)UART7_BASEADDR)
#define UART8     ((USART_RegDef_t*)UART8_BASEADDR)

//clock control macros

#define USART1_PCLK_EN()     (RCC->APB2ENR |= (1 << 4))
#define USART6_PCLK_EN()     (RCC->APB2ENR |= (1 << 5))
#define USART2_PCLK_EN()     (RCC->APB1ENR |= (1 << 17))
#define USART3_PCLK_EN()     (RCC->APB1ENR |= (1 << 18))
#define UART4_PCLK_EN()      (RCC->APB1ENR |= (1 << 19))
#define UART5_PCLK_EN()      (RCC->APB1ENR |= (1 << 20))
#define UART7_PCLK_EN()      (RCC->APB1ENR |= (1 << 30))
#define UART8_PCLK_EN()      (RCC->APB1ENR |= (1 << 31))

#define USART1_PCLK_DI()     (RCC->APB2ENR &= ~(1 << 4))
#define USART6_PCLK_DI()     (RCC->APB2ENR &= ~(1 << 5))

#define USART2_PCLK_DI()     (RCC->APB1ENR &= ~(1 << 17))
#define USART3_PCLK_DI()     (RCC->APB1ENR &= ~(1 << 18))
#define UART4_PCLK_DI()      (RCC->APB1ENR &= ~(1 << 19))
#define UART5_PCLK_DI()      (RCC->APB1ENR &= ~(1 << 20))
#define UART7_PCLK_DI()      (RCC->APB1ENR &= ~(1 << 30))
#define UART8_PCLK_DI()      (RCC->APB1ENR &= ~(1 << 31))

#define USART1_REG_RESET() do {RCC->APB2RSTR |= (1 << 4); RCC->APB2RSTR &= ~(1 << 4); } while(0)
#define USART6_REG_RESET() do {RCC->APB2RSTR |= (1 << 5); RCC->APB2RSTR &= ~(1 << 5); } while(0)
#define USART2_REG_RESET() do {RCC->APB1RSTR |= (1 << 17);RCC->APB1RSTR &= ~(1 << 17);} while(0)
#define USART3_REG_RESET() do {RCC->APB1RSTR |= (1 << 18);RCC->APB1RSTR &= ~(1 << 18);} while(0)
#define UART4_REG_RESET() do {RCC->APB1RSTR |= (1 << 19); RCC->APB1RSTR &= ~(1 << 19);} while(0)
#define UART5_REG_RESET() do {RCC->APB1RSTR |= (1 << 20); RCC->APB1RSTR &= ~(1 << 20);} while(0)
#define UART7_REG_RESET() do {RCC->APB1RSTR |= (1 << 30); RCC->APB1RSTR &= ~(1 << 30);} while(0)
#define UART8_REG_RESET() do {RCC->APB1RSTR |= (1 << 31); RCC->APB1RSTR &= ~(1 << 31);} while(0)

//uart registers

#define USART_SR_PE       0
#define USART_SR_FE       1
#define USART_SR_NE       2
#define USART_SR_ORE      3
#define USART_SR_IDLE     4
#define USART_SR_RXNE     5
#define USART_SR_TC       6
#define USART_SR_TXE      7
#define USART_SR_LBD      8
#define USART_SR_CTS      9

#define USART_CR1_SBK       0
#define USART_CR1_RWU       1
#define USART_CR1_RE        2
#define USART_CR1_TE        3
#define USART_CR1_IDLEIE    4
#define USART_CR1_RXNEIE    5
#define USART_CR1_TCIE      6
#define USART_CR1_TXEIE     7
#define USART_CR1_PEIE      8
#define USART_CR1_PS        9
#define USART_CR1_PCE       10
#define USART_CR1_WAKE      11
#define USART_CR1_M         12
#define USART_CR1_UE        13
#define USART_CR1_OVER8     15

#define USART_CR2_ADD       0
#define USART_CR2_LBDL      5
#define USART_CR2_LBDIE     6
#define USART_CR2_LBCL      8
#define USART_CR2_CPHA      9
#define USART_CR2_CPOL      10
#define USART_CR2_CLKEN     11
#define USART_CR2_STOP      12
#define USART_CR2_LINEN     14

#define USART_CR3_EIE       0
#define USART_CR3_IREN      1
#define USART_CR3_IRLP      2
#define USART_CR3_HDSEL     3
#define USART_CR3_NACK      4
#define USART_CR3_SCEN      5
#define USART_CR3_DMAR      6
#define USART_CR3_DMAT      7
#define USART_CR3_RTSE      8
#define USART_CR3_CTSE      9
#define USART_CR3_CTSIE     10

#define IRQ_NO_USART1    37
#define IRQ_NO_USART2    38
#define IRQ_NO_USART3    39
#define IRQ_NO_UART4     52
#define IRQ_NO_UART5     53
#define IRQ_NO_USART6    71
#define IRQ_NO_UART7     82
#define IRQ_NO_UART8     83





//9. RCC register definition

typedef struct
{
    __vo uint32_t CR;
    __vo uint32_t PLLCFGR;
    __vo uint32_t CFGR;
    __vo uint32_t CIR;
    __vo uint32_t AHB1RSTR;
    __vo uint32_t AHB2RSTR;
    __vo uint32_t AHB3RSTR;
    uint32_t      RESERVED0;
    __vo uint32_t APB1RSTR;
    __vo uint32_t APB2RSTR;
    uint32_t      RESERVED1[2];
    __vo uint32_t AHB1ENR;
    __vo uint32_t AHB2ENR;
    __vo uint32_t AHB3ENR;
    uint32_t      RESERVED2;
    __vo uint32_t APB1ENR;
    __vo uint32_t APB2ENR;
    uint32_t      RESERVED3[2];
    __vo uint32_t AHB1LPENR;
    __vo uint32_t AHB2LPENR;
    __vo uint32_t AHB3LPENR;
    uint32_t      RESERVED4;
    __vo uint32_t APB1LPENR;
    __vo uint32_t APB2LPENR;
    uint32_t      RESERVED5[2];
    __vo uint32_t BDCR;
    __vo uint32_t CSR;
    uint32_t      RESERVED6[2];
    __vo uint32_t SSCGR;
    __vo uint32_t PLLI2SCFGR;
    __vo uint32_t PLLSAICFGR;
    __vo uint32_t DCKCFGR;
    __vo uint32_t CKGATENR;
    __vo uint32_t DCKCFGR2;
} RCC_RegDef_t;

#define RCC             ((RCC_RegDef_t*)RCC_BASEADDR)

//10. EXTI register definition

typedef struct
{
    __vo uint32_t IMR;
    __vo uint32_t EMR;
    __vo uint32_t RTSR;
    __vo uint32_t FTSR;
    __vo uint32_t SWIER;
    __vo uint32_t PR;
} EXTI_RegDef_t;

#define EXTI_BASEADDR    (APB2PERIPH_BASEADDR + 0x3C00)
#define EXTI             ((EXTI_RegDef_t*)EXTI_BASEADDR)

#define EXTI_LINE0      0
#define EXTI_LINE1      1
#define EXTI_LINE2      2
#define EXTI_LINE3      3
#define EXTI_LINE4      4
#define EXTI_LINE5      5
#define EXTI_LINE6      6
#define EXTI_LINE7      7
#define EXTI_LINE8      8
#define EXTI_LINE9      9
#define EXTI_LINE10     10
#define EXTI_LINE11     11
#define EXTI_LINE12     12
#define EXTI_LINE13     13
#define EXTI_LINE14     14
#define EXTI_LINE15     15

//11. SYSCFG register definition

typedef struct
{
    __vo uint32_t MEMRMP;
    __vo uint32_t PMC;
    __vo uint32_t EXTICR[4];
    uint32_t RESERVED1[2];
    __vo uint32_t CMPCR;
    uint32_t RESERVED2[2];
    __vo uint32_t CFGR;
} SYSCFG_RegDef_t;

#define SYSCFG_BASEADDR    (APB2PERIPH_BASEADDR + 0x3800)
#define SYSCFG             ((SYSCFG_RegDef_t*)SYSCFG_BASEADDR)

//14. Peripheral reset macros
//15. Generic macros
//16. IRQ numbers
//19. USART bit definitions
//20. Include driver headers

//some generic macros
#define ENABLE					1
#define DISABLE					0
#define SET						ENABLE
#define RESET					DISABLE
#define GPIO_PIN_SET			SET
#define GPIO_PIN_RESET			RESET
#define FLAG_RESET				RESET
#define FLAG_SET				SET

#include "stm32f429xx_driver_gpio.h"
#include "stm32f429xx_driver_uart.h"
#include "stm32f429xx_driver_rcc.h"







#endif /* INC_STM32F429XX_H_ */
