################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src2/soc/amlogic/s805.c \
../src2/soc/amlogic/s905.c 

OBJS += \
./src2/soc/amlogic/s805.o \
./src2/soc/amlogic/s905.o 

C_DEPS += \
./src2/soc/amlogic/s805.d \
./src2/soc/amlogic/s905.d 


# Each subdirectory must supply rules for building sources it contributes
src2/soc/amlogic/%.o: ../src2/soc/amlogic/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I/usr/arm-linux-gnueabihf/include -I"/home/echocare/GitRepositories/Novelda/src2/soc/nxp" -I/home/echocare/GitRepositories/wiringX/src -I"/home/echocare/GitRepositories/Novelda/include" -I"/home/echocare/GitRepositories/Novelda/src2" -I"/home/echocare/GitRepositories/Novelda/src/Radar" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


