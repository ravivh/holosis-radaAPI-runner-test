################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Radar/taskRadar.cpp 

OBJS += \
./src/Radar/taskRadar.o 

CPP_DEPS += \
./src/Radar/taskRadar.d 


# Each subdirectory must supply rules for building sources it contributes
src/Radar/%.o: ../src/Radar/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-gnueabihf-g++ -D__cplusplus=201103L -I/usr/arm-linux-gnueabihf/include -I"/home/echocare/GitRepositories/Novelda/src2/soc/nxp" -I"/home/echocare/GitRepositories/Novelda/src/Radar" -I"/home/echocare/GitRepositories/Novelda/src2" -I"/home/echocare/GitRepositories/Novelda/src" -I"/home/echocare/GitRepositories/Novelda/src/hal" -I"/home/echocare/GitRepositories/Novelda/src/XDriver" -I"/home/echocare/GitRepositories/Novelda/include" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


