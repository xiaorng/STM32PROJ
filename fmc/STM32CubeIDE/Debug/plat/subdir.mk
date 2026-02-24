################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../plat/cli.c \
../plat/fmac_rt.c \
../plat/log.c 

OBJS += \
./plat/cli.o \
./plat/fmac_rt.o \
./plat/log.o 

C_DEPS += \
./plat/cli.d \
./plat/fmac_rt.d \
./plat/log.d 


# Each subdirectory must supply rules for building sources it contributes
plat/%.o plat/%.su plat/%.cyclo: ../plat/%.c plat/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DARM_MATH_CM4 -DUSE_HAL_DRIVER -DSTM32G474xx -c -I../../Inc -I../../Drivers/STM32G4xx_HAL_Driver/Inc -I../../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v6.4.1-Full/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v6.4.1-Full/MotorControl/MCSDK/MCLib/G4xx/Inc -I../../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/CMSIS/DSP/Include -I"E:/STM/MOTOR/foc-fmac-coric/fmc/STM32CubeIDE/plat" -I"E:/STM/MOTOR/foc-fmac-coric/fmc/STM32CubeIDE/Bsp" -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-plat

clean-plat:
	-$(RM) ./plat/cli.cyclo ./plat/cli.d ./plat/cli.o ./plat/cli.su ./plat/fmac_rt.cyclo ./plat/fmac_rt.d ./plat/fmac_rt.o ./plat/fmac_rt.su ./plat/log.cyclo ./plat/log.d ./plat/log.o ./plat/log.su

.PHONY: clean-plat

