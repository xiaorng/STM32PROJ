################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Bsp/bsp_uart.c 

OBJS += \
./Bsp/bsp_uart.o 

C_DEPS += \
./Bsp/bsp_uart.d 


# Each subdirectory must supply rules for building sources it contributes
Bsp/%.o Bsp/%.su Bsp/%.cyclo: ../Bsp/%.c Bsp/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DARM_MATH_CM4 -DUSE_HAL_DRIVER -DSTM32G474xx -c -I../../Inc -I../../Drivers/STM32G4xx_HAL_Driver/Inc -I../../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v6.4.1-Full/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v6.4.1-Full/MotorControl/MCSDK/MCLib/G4xx/Inc -I../../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -I"E:/STM/MOTOR/foc-fmac-coric/fmc/STM32CubeIDE/plat" -I"E:/STM/MOTOR/foc-fmac-coric/fmc/STM32CubeIDE/Bsp" -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Bsp

clean-Bsp:
	-$(RM) ./Bsp/bsp_uart.cyclo ./Bsp/bsp_uart.d ./Bsp/bsp_uart.o ./Bsp/bsp_uart.su

.PHONY: clean-Bsp

