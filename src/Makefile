TARGET = firmware
 
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
 
# 1. FIXED: Added ../ to go up one folder to reach Drivers/Inc
INCLUDES = -I"../Drivers/Inc"
INCLUDES = -I"../jrd100"
MCU_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
 
CFLAGS  = $(MCU_FLAGS) -Wall -O0 -g -std=gnu11 -ffunction-sections -fdata-sections
CFLAGS += $(INCLUDES)

LDFLAGS = $(MCU_FLAGS) -T ../STM32F429ZITX_FLASH.ld -nostdlib -Wl,--gc-sections -Wl,-Map=$(TARGET).map
 
# 2. FIXED: Use ../ to correctly find your driver source files
SRCS_C  = $(wildcard *.c)
SRCS_C += ../Drivers/src/stm32f429xx_driver_rcc.c
SRCS_C += ../Drivers/src/stm32f429xx_gpio_driver.c
SRCS_C += ../Drivers/src/stm32f429xx_usart_driver.c

# If startup file is also in the parent folder, add ../ here too
SRCS_S  = startup_stm32f429xx.s

OBJS    = $(SRCS_C:.c=.o) $(SRCS_S:.s=.o)

all: $(TARGET).elf
 
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
 
%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@
 
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) $@
 
# 3. FIXED: Clean path updated for driver objects
clean:
	rm -f *.o ../Drivers/src/*.o $(TARGET).elf $(TARGET).map
 
.PHONY: all clean