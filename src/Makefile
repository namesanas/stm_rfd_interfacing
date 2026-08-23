TARGET = firmware
 
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
 
# 1. Include paths (Keep the -I flag)
INCLUDES = -I"stm_rfd_interfacing/Drivers/Inc"

MCU_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
 
# 2. FIXED: Combine flags correctly using += so INCLUDES isn't erased
CFLAGS  = $(MCU_FLAGS) -Wall -O0 -g -std=gnu11 -ffunction-sections -fdata-sections
CFLAGS += $(INCLUDES)

LDFLAGS = $(MCU_FLAGS) -T STM32F429ZITX_FLASH.ld -nostdlib -Wl,--gc-sections -Wl,-Map=$(TARGET).map
 
# 3. FIXED: Use += to add files together without overwriting them
SRCS_C  = $(wildcard *.c)
SRCS_C += Drivers/src/stm32f429xx_driver_rcc.c
SRCS_C += Drivers/src/stm32f429xx_gpio_driver.c
SRCS_C += Drivers/src/stm32f429xx_usart_driver.c

SRCS_S  = startup_stm32f429xx.s

# 4. FIXED: Generate object files for ALL .c files including drivers
OBJS    = $(SRCS_C:.c=.o) $(SRCS_S:.s=.o)

all: $(TARGET).elf
 
# 5. FIXED: Added dependency folder support to the compilation rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
 
%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@
 
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) $@
 
# 6. FIXED: Expanded clean to remove driver object files too
clean:
	rm -f *.o Drivers/src/*.o $(TARGET).elf $(TARGET).map
 
.PHONY: all clean