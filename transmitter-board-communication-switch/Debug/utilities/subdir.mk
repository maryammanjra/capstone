################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utilities/fsl_debug_console.c 

C_DEPS += \
./utilities/fsl_debug_console.d 

OBJS += \
./utilities/fsl_debug_console.o 


# Each subdirectory must supply rules for building sources it contributes
utilities/%.o: ../utilities/%.c utilities/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MKL25Z128VLK4 -DCPU_MKL25Z128VLK4_cm0plus -DSDK_OS_BAREMETAL -DFSL_RTOS_BM -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/transmitter-board-communication-switch/board" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/transmitter-board-communication-switch/source" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/transmitter-board-communication-switch" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/transmitter-board-communication-switch/drivers" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/transmitter-board-communication-switch/startup" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/transmitter-board-communication-switch/utilities" -I"/Users/maryam/Documents/MCUXpressoIDE_25.6.136/workspace/transmitter-board-communication-switch/CMSIS" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-utilities

clean-utilities:
	-$(RM) ./utilities/fsl_debug_console.d ./utilities/fsl_debug_console.o

.PHONY: clean-utilities

