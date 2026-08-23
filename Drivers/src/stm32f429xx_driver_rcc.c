/*
 * stm32f429xx_rcc_driver.c
 *
 * Created on: Jul 16, 2026
 * Author: anas
 */

#include "stm32f429xx_driver_rcc.h"


uint16_t AHB_PreScaler[8] =
{
    2, 4, 8, 16, 64, 128, 256, 512
};


uint16_t APB_PreScaler[4] =
{
    2, 4, 8, 16
};


/*
 * @fn             - RCC_GetPLLOutputClock
 *
 * @brief          - Returns PLL output clock frequency
 */
uint32_t RCC_GetPLLOutputClock(void)
{
    uint32_t pllm;
    uint32_t plln;
    uint32_t pllp;

    uint32_t vco_input_frequency;
    uint32_t vco_output_frequency;
    uint32_t pll_output_frequency;

    uint32_t pllsrc;


    /*
     * PLLM
     * Bits [5:0]
     */
    pllm = RCC->PLLCFGR & 0x3F;


    /*
     * PLLN
     * Bits [14:6]
     */
    plln = (RCC->PLLCFGR >> 6) & 0x1FF;


    /*
     * PLLP
     *
     * Bits [17:16]
     *
     * Encoding:
     *
     * 00 -> /2
     * 01 -> /4
     * 10 -> /6
     * 11 -> /8
     */
    pllp = ((RCC->PLLCFGR >> 16) & 0x3);

    pllp = (pllp * 2) + 2;


    /*
     * PLL source
     *
     * 0 -> HSI
     * 1 -> HSE
     */
    pllsrc = ((RCC->PLLCFGR >> 22) & 0x1);


    if(pllsrc == 0)
    {
        /*
         * HSI = 16 MHz
         */
        vco_input_frequency = 16000000 / pllm;
    }
    else
    {
        /*
         * HSE frequency depends on the board.
         *
         * Change this value if your hardware
         * uses a different HSE crystal.
         */
        vco_input_frequency = 8000000 / pllm;
    }


    vco_output_frequency =
        vco_input_frequency * plln;


    pll_output_frequency =
        vco_output_frequency / pllp;


    return pll_output_frequency;
}


/*
 * @fn             - RCC_GetSystemClockValue
 *
 * @brief          - Returns system clock frequency
 */
uint32_t RCC_GetSystemClockValue(void)
{
    uint32_t clksrc;
    uint32_t SystemClk;


    /*
     * SWS bits [3:2]
     *
     * 00 -> HSI
     * 01 -> HSE
     * 10 -> PLL
     */
    clksrc = ((RCC->CFGR >> 2) & 0x3);


    if(clksrc == 0)
    {
        SystemClk = 16000000;
    }
    else if(clksrc == 1)
    {
        /*
         * HSE assumed to be 8 MHz.
         */
        SystemClk = 8000000;
    }
    else if(clksrc == 2)
    {
        SystemClk = RCC_GetPLLOutputClock();
    }
    else
    {
        SystemClk = 16000000;
    }


    return SystemClk;
}


/*
 * @fn             - RCC_GetPCLK1Value
 *
 * @brief          - Returns APB1 peripheral clock frequency
 */
uint32_t RCC_GetPCLK1Value(void)
{
    uint32_t pclk1;
    uint32_t SystemClk;

    uint32_t temp;
    uint32_t ahbp;
    uint32_t apb1p;


    SystemClk = RCC_GetSystemClockValue();


    /*
     * AHB prescaler
     *
     * HPRE bits [7:4]
     */
    temp = ((RCC->CFGR >> 4) & 0xF);


    if(temp < 8)
    {
        ahbp = 1;
    }
    else
    {
        ahbp = AHB_PreScaler[temp - 8];
    }


    /*
     * APB1 prescaler
     *
     * PPRE1 bits [12:10]
     */
    temp = ((RCC->CFGR >> 10) & 0x7);


    if(temp < 4)
    {
        apb1p = 1;
    }
    else
    {
        apb1p = APB_PreScaler[temp - 4];
    }


    pclk1 = (SystemClk / ahbp) / apb1p;


    return pclk1;
}


/*
 * @fn             - RCC_GetPCLK2Value
 *
 * @brief          - Returns APB2 peripheral clock frequency
 */
uint32_t RCC_GetPCLK2Value(void)
{
    uint32_t pclk2;
    uint32_t SystemClk;

    uint32_t temp;
    uint32_t ahbp;
    uint32_t apb2p;


    SystemClk = RCC_GetSystemClockValue();


    /*
     * AHB prescaler
     *
     * HPRE bits [7:4]
     */
    temp = ((RCC->CFGR >> 4) & 0xF);


    if(temp < 8)
    {
        ahbp = 1;
    }
    else
    {
        ahbp = AHB_PreScaler[temp - 8];
    }


    /*
     * APB2 prescaler
     *
     * PPRE2 bits [15:13]
     */
    temp = ((RCC->CFGR >> 13) & 0x7);


    if(temp < 4)
    {
        apb2p = 1;
    }
    else
    {
        apb2p = APB_PreScaler[temp - 4];
    }


    pclk2 = (SystemClk / ahbp) / apb2p;


    return pclk2;
}

