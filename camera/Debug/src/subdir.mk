################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/I2CWrapper.c \
../src/audioReceiveThread.c \
../src/audioTransmitThread.c \
../src/connections.c \
../src/i2cThread.c \
../src/i2c_funcs.c \
../src/io.c \
../src/keys.c \
../src/main.c \
../src/mcp23017.c \
../src/ringThread.c \
../src/testThread.c \
../src/timerThread.c \
../src/updateThread.c \
../src/videoThread.c 

C_DEPS += \
./src/I2CWrapper.d \
./src/audioReceiveThread.d \
./src/audioTransmitThread.d \
./src/connections.d \
./src/i2cThread.d \
./src/i2c_funcs.d \
./src/io.d \
./src/keys.d \
./src/main.d \
./src/mcp23017.d \
./src/ringThread.d \
./src/testThread.d \
./src/timerThread.d \
./src/updateThread.d \
./src/videoThread.d 

OBJS += \
./src/I2CWrapper.o \
./src/audioReceiveThread.o \
./src/audioTransmitThread.o \
./src/connections.o \
./src/i2cThread.o \
./src/i2c_funcs.o \
./src/io.o \
./src/keys.o \
./src/main.o \
./src/mcp23017.o \
./src/ringThread.o \
./src/testThread.o \
./src/timerThread.o \
./src/updateThread.o \
./src/videoThread.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-none-linux-gnueabihf-gcc -std=c99 -D_GNU_SOURCE -I/mnt/linuxData/projecten/nanopi/sys-root//usr/include -I/mnt/linuxData/projecten/nanopi/sys-root//usr/include/arm-linux-gnueabihf -I/mnt/linuxData/projecten/nanopi/sys-root//usr/lib/arm-linux-gnueabihf/glib-2.0/include -I/mnt/linuxData/projecten/nanopi/sys-root//usr/include/glib-2.0/include -I/mnt/linuxData/projecten/nanopi/sys-root//usr/include/glib-2.0 -I/mnt/linuxData/projecten/nanopi/sys-root//usr/include/gstreamer-1.0 -O0 -g3 -Wall -c -fmessage-length=0 --sysroot=/mnt/linuxData/nanoPiFire2/sysroot -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/I2CWrapper.d ./src/I2CWrapper.o ./src/audioReceiveThread.d ./src/audioReceiveThread.o ./src/audioTransmitThread.d ./src/audioTransmitThread.o ./src/connections.d ./src/connections.o ./src/i2cThread.d ./src/i2cThread.o ./src/i2c_funcs.d ./src/i2c_funcs.o ./src/io.d ./src/io.o ./src/keys.d ./src/keys.o ./src/main.d ./src/main.o ./src/mcp23017.d ./src/mcp23017.o ./src/ringThread.d ./src/ringThread.o ./src/testThread.d ./src/testThread.o ./src/timerThread.d ./src/timerThread.o ./src/updateThread.d ./src/updateThread.o ./src/videoThread.d ./src/videoThread.o

.PHONY: clean-src

