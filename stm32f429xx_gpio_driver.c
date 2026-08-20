void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi)
{
    if(EnorDi == ENABLE)
    {
        if(pGPIOx == GPIOA)
        {
            GPIOA_PCLK_EN();
        }
        else if(pGPIOx == GPIOB)
        {
            GPIOB_PCLK_EN();
        }
        else if(pGPIOx == GPIOC)
        {
            GPIOC_PCLK_EN();
        }
        else if(pGPIOx == GPIOD)
        {
            GPIOD_PCLK_EN();
        }
        else if(pGPIOx == GPIOE)
        {
            GPIOE_PCLK_EN();
        }
        else if(pGPIOx == GPIOF)
        {
            GPIOF_PCLK_EN();
        }
        else if(pGPIOx == GPIOG)
        {
            GPIOG_PCLK_EN();
        }
        else if(pGPIOx == GPIOH)
        {
            GPIOH_PCLK_EN();
        }
        else if(pGPIOx == GPIOI)
        {
            GPIOI_PCLK_EN();
        }
        else if(pGPIOx == GPIOJ)
        {
            GPIOJ_PCLK_EN();
        }
        else if(pGPIOx == GPIOK)
        {
            GPIOK_PCLK_EN();
        }
    }
    else
    {
        if(pGPIOx == GPIOA)
        {
            GPIOA_PCLK_DI();
        }
        else if(pGPIOx == GPIOB)
        {
            GPIOB_PCLK_DI();
        }
        else if(pGPIOx == GPIOC)
        {
            GPIOC_PCLK_DI();
        }
        else if(pGPIOx == GPIOD)
        {
            GPIOD_PCLK_DI();
        }
        else if(pGPIOx == GPIOE)
        {
            GPIOE_PCLK_DI();
        }
        else if(pGPIOx == GPIOF)
        {
            GPIOF_PCLK_DI();
        }
        else if(pGPIOx == GPIOG)
        {
            GPIOG_PCLK_DI();
        }
        else if(pGPIOx == GPIOH)
        {
            GPIOH_PCLK_DI();
        }
        else if(pGPIOx == GPIOI)
        {
            GPIOI_PCLK_DI();
        }
        else if(pGPIOx == GPIOJ)
        {
            GPIOJ_PCLK_DI();
        }
        else if(pGPIOx == GPIOK)
        {
            GPIOK_PCLK_DI();
        }
    }
}

void GPIO_Init(GPIO_Handle_t *pGPIOHandle)
{
    uint32_t temp = 0;

    /* Enable peripheral clock */
    GPIO_PeriClockControl(pGPIOHandle->pGPIOx, ENABLE);


    /*
     * Configure GPIO mode
     */

    if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_ANALOG)
    {
        temp = (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode
                << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));

        pGPIOHandle->pGPIOx->MODER &= ~(0x3 <<
                (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));

        pGPIOHandle->pGPIOx->MODER |= temp;
    }
    else
    {
        /*
         * Interrupt mode
         */

        if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_FT)
        {
            EXTI->FTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
            EXTI->RTSR &= ~(1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        }
        else if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RT)
        {
            EXTI->RTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
            EXTI->FTSR &= ~(1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        }
        else if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RFT)
        {
            EXTI->RTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
            EXTI->FTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
        }

        /*
         * Configure GPIO port as input for EXTI
         */
        pGPIOHandle->pGPIOx->MODER &= ~(0x3 <<
                (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));

        /*
         * Enable interrupt request
         */
        EXTI->IMR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);

        /*
         * Configure SYSCFG EXTICR
         */
        SYSCFG_PCLK_EN();

        uint8_t temp1 =
            pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 4;

        uint8_t temp2 =
            pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 4;

        uint8_t portcode = 0;

        if(pGPIOHandle->pGPIOx == GPIOA)
        {
            portcode = GPIO_EXTI_PORTA;
        }
        else if(pGPIOHandle->pGPIOx == GPIOB)
        {
            portcode = GPIO_EXTI_PORTB;
        }
        else if(pGPIOHandle->pGPIOx == GPIOC)
        {
            portcode = GPIO_EXTI_PORTC;
        }
        else if(pGPIOHandle->pGPIOx == GPIOD)
        {
            portcode = GPIO_EXTI_PORTD;
        }
        else if(pGPIOHandle->pGPIOx == GPIOE)
        {
            portcode = GPIO_EXTI_PORTE;
        }
        else if(pGPIOHandle->pGPIOx == GPIOF)
        {
            portcode = GPIO_EXTI_PORTF;
        }
        else if(pGPIOHandle->pGPIOx == GPIOG)
        {
            portcode = GPIO_EXTI_PORTG;
        }
        else if(pGPIOHandle->pGPIOx == GPIOH)
        {
            portcode = GPIO_EXTI_PORTH;
        }
        else if(pGPIOHandle->pGPIOx == GPIOI)
        {
            portcode = GPIO_EXTI_PORTI;
        }

        /*
         * Clear the existing 4-bit EXTICR field first.
         */
        SYSCFG->EXTICR[temp1] &= ~(0xF << (temp2 * 4));

        /*
         * Select GPIO port for this EXTI line.
         */
        SYSCFG->EXTICR[temp1] |=
            (portcode << (temp2 * 4));
    }


    /*
     * Configure output type
     */

    temp = pGPIOHandle->GPIO_PinConfig.GPIO_PinOPType
           << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;

    pGPIOHandle->pGPIOx->OTYPER &= ~(0x1 <<
            pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);

    pGPIOHandle->pGPIOx->OTYPER |= temp;


    /*
     * Configure output speed
     */

    temp = pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed
           << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);

    pGPIOHandle->pGPIOx->OSPEEDR &= ~(0x3 <<
            (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));

    pGPIOHandle->pGPIOx->OSPEEDR |= temp;


    /*
     * Configure pull-up / pull-down
     */

    temp = pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl
           << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);

    pGPIOHandle->pGPIOx->PUPDR &= ~(0x3 <<
            (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber));

    pGPIOHandle->pGPIOx->PUPDR |= temp;


    /*
     * Configure alternate function
     */

    if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode ==
       GPIO_MODE_ALTFN)
    {
        uint8_t temp1;
        uint8_t temp2;

        temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 8;
        temp2 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 8;

        pGPIOHandle->pGPIOx->AFR[temp1] &= ~(0xF <<
                (temp2 * 4));

        pGPIOHandle->pGPIOx->AFR[temp1] |=
            (pGPIOHandle->GPIO_PinConfig.GPIO_PinAltFunMode
             << (temp2 * 4));
    }
}

