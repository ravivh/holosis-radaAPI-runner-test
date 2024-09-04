################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src2/platform/hardkernel/odroidc1.c \
../src2/platform/hardkernel/odroidc2.c \
../src2/platform/hardkernel/odroidxu4.c 

OBJS += \
./src2/platform/hardkernel/odroidc1.o \
./src2/platform/hardkernel/odroidc2.o \
./src2/platform/hardkernel/odroidxu4.o 

C_DEPS += \
./src2/platform/hardkernel/odroidc1.d \
./src2/platform/hardkernel/odroidc2.d \
./src2/platform/hardkernel/odroidxu4.d 


# Each subdirectory must supply rules for building sources it contributes
src2/platform/hardkernel/%.o: ../src2/platform/hardkernel/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I/usr/arm-linux-gnueabihf/include -I/home/echocare/GitRepositories/wiringX/src -I"//home/echocare/GitRepositories/Novelda Matlab/src/Radar" -I"//home/echocare/GitRepositories/Novelda Matlab/include" -I"//home/echocare/GitRepositories/Novelda Matlab/src2" -I"//home/echocare/GitRepositories/Novelda Matlab/src2/soc/nxp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


