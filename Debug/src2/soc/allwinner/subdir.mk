################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src2/soc/allwinner/a10.c \
../src2/soc/allwinner/a31s.c \
../src2/soc/allwinner/h3.c \
../src2/soc/allwinner/h5.c 

OBJS += \
./src2/soc/allwinner/a10.o \
./src2/soc/allwinner/a31s.o \
./src2/soc/allwinner/h3.o \
./src2/soc/allwinner/h5.o 

C_DEPS += \
./src2/soc/allwinner/a10.d \
./src2/soc/allwinner/a31s.d \
./src2/soc/allwinner/h3.d \
./src2/soc/allwinner/h5.d 


# Each subdirectory must supply rules for building sources it contributes
src2/soc/allwinner/%.o: ../src2/soc/allwinner/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I/usr/arm-linux-gnueabihf/include -I/home/echocare/GitRepositories/wiringX/src -I"//home/echocare/GitRepositories/Novelda Matlab/src/Radar" -I"//home/echocare/GitRepositories/Novelda Matlab/include" -I"//home/echocare/GitRepositories/Novelda Matlab/src2" -I"//home/echocare/GitRepositories/Novelda Matlab/src2/soc/nxp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


