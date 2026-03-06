################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/LCD_4bit.c \
../source/LEDs.c \
../source/delay.c \
../source/main.c \
../source/mtb.c \
../source/semihost_hardfault.c \
../source/timers.c 

C_DEPS += \
./source/LCD_4bit.d \
./source/LEDs.d \
./source/delay.d \
./source/main.d \
./source/mtb.d \
./source/semihost_hardfault.d \
./source/timers.d 

OBJS += \
./source/LCD_4bit.o \
./source/LEDs.o \
./source/delay.o \
./source/main.o \
./source/mtb.o \
./source/semihost_hardfault.o \
./source/timers.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MKL25Z128VLK4 -DCPU_MKL25Z128VLK4_cm0plus -DSDK_OS_BAREMETAL -DFSL_RTOS_BM -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/reciever-board-communication-switch/board" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/reciever-board-communication-switch/source" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/reciever-board-communication-switch" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/reciever-board-communication-switch/drivers" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/reciever-board-communication-switch/startup" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/reciever-board-communication-switch/utilities" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/reciever-board-communication-switch/CMSIS" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source

clean-source:
	-$(RM) ./source/LCD_4bit.d ./source/LCD_4bit.o ./source/LEDs.d ./source/LEDs.o ./source/delay.d ./source/delay.o ./source/main.d ./source/main.o ./source/mtb.d ./source/mtb.o ./source/semihost_hardfault.d ./source/semihost_hardfault.o ./source/timers.d ./source/timers.o

.PHONY: clean-source

