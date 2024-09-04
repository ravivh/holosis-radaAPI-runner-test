################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hal/radar_hal.c 

OBJS += \
./src/hal/radar_hal.o 

C_DEPS += \
./src/hal/radar_hal.d 


# Each subdirectory must supply rules for building sources it contributes
src/hal/%.o: ../src/hal/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I/usr/arm-linux-gnueabihf/include -I"/home/echocare/GitRepositories/Novelda/src2/soc/nxp" -I/home/echocare/GitRepositories/wiringX/src -I"/home/echocare/GitRepositories/Novelda/include" -I"/home/echocare/GitRepositories/Novelda/src2" -I"/home/echocare/GitRepositories/Novelda/src/Radar" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


