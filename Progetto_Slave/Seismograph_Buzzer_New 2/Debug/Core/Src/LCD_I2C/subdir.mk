################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/LCD_I2C/i2c-lcd.c 

OBJS += \
./Core/Src/LCD_I2C/i2c-lcd.o 

C_DEPS += \
./Core/Src/LCD_I2C/i2c-lcd.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/LCD_I2C/%.o Core/Src/LCD_I2C/%.su Core/Src/LCD_I2C/%.cyclo: ../Core/Src/LCD_I2C/%.c Core/Src/LCD_I2C/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F303xC -c -I../Core/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F3xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/ASUS/Desktop/Seismograph_Buzzer_New/Core/Src/LCD_I2C" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-LCD_I2C

clean-Core-2f-Src-2f-LCD_I2C:
	-$(RM) ./Core/Src/LCD_I2C/i2c-lcd.cyclo ./Core/Src/LCD_I2C/i2c-lcd.d ./Core/Src/LCD_I2C/i2c-lcd.o ./Core/Src/LCD_I2C/i2c-lcd.su

.PHONY: clean-Core-2f-Src-2f-LCD_I2C

