################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/audioReceiveThread.cpp \
../src/audioTransmitThread.cpp \
../src/connections.cpp \
../src/io.cpp \
../src/keys.cpp \
../src/main.cpp \
../src/testThread.cpp \
../src/timerThread.cpp \
../src/updateThread.cpp \
../src/videoThread.cpp 

CPP_DEPS += \
./src/audioReceiveThread.d \
./src/audioTransmitThread.d \
./src/connections.d \
./src/io.d \
./src/keys.d \
./src/main.d \
./src/testThread.d \
./src/timerThread.d \
./src/updateThread.d \
./src/videoThread.d 

OBJS += \
./src/audioReceiveThread.o \
./src/audioTransmitThread.o \
./src/connections.o \
./src/io.o \
./src/keys.o \
./src/main.o \
./src/testThread.o \
./src/timerThread.o \
./src/updateThread.o \
./src/videoThread.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-none-linux-gnueabihfg++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/audioReceiveThread.d ./src/audioReceiveThread.o ./src/audioTransmitThread.d ./src/audioTransmitThread.o ./src/connections.d ./src/connections.o ./src/io.d ./src/io.o ./src/keys.d ./src/keys.o ./src/main.d ./src/main.o ./src/testThread.d ./src/testThread.o ./src/timerThread.d ./src/timerThread.o ./src/updateThread.d ./src/updateThread.o ./src/videoThread.d ./src/videoThread.o

.PHONY: clean-src

