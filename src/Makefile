
TARGET = firmware
 
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
 
MCU_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
 
CFLAGS  = $(MCU_FLAGS) -Wall -O0 -g -std=gnu11 -ffunction-sections -fdata-sections
LDFLAGS = $(MCU_FLAGS) -T STM32F429ZITX_FLASH.ld -nostdlib -Wl,--gc-sections -Wl,-Map=$(TARGET).map
 
# Picks up every .c file in this folder automatically, plus the startup file
SRCS_C = $(wildcard *.c)
SRCS_S = startup_stm32f429xx.s
OBJS   = $(SRCS_C:.c=.o) $(SRCS_S:.s=.o)
 
all: $(TARGET).elf
 
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
 
%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@
 
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) $@
 
clean:
	rm -f *.o $(TARGET).elf $(TARGET).map
 
.PHONY: all clean
 
