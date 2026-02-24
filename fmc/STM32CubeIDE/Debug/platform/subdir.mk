################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../platform/cli.c \
../platform/fmac_rt.c \
../platform/log.c 

OBJS += \
./platform/cli.o \
./platform/fmac_rt.o \
./platform/log.o 

C_DEPS += \
./platform/cli.d \
./platform/fmac_rt.d \
./platform/log.d 


# Each subdirectory must supply rules for building sources it contributes
platform/%.o platform/%.su platform/%.cyclo: ../platform/%.c platform/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DARM_MATH_CM4 -DUSE_HAL_DRIVER -DSTM32G474xx -c -I../../Inc -I../../Drivers/STM32G4xx_HAL_Driver/Inc -I../../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../../MCSDK_v6.4.1-Full/MotorControl/MCSDK/MCLib/Any/Inc -I../../MCSDK_v6.4.1-Full/MotorControl/MCSDK/MCLib/G4xx/Inc -I../../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../../Drivers/CMSIS/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-platform

clean-platform:
	-$(RM) ./platform/cli.cyclo ./platform/cli.d ./platform/cli.o ./platform/cli.su ./platform/fmac_rt.cyclo ./platform/fmac_rt.d ./platform/fmac_rt.o ./platform/fmac_rt.su ./platform/log.cyclo ./platform/log.d ./platform/log.o ./platform/log.su

.PHONY: clean-platform

