################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src2/platform/solidrun/hummingboard_base_pro_dq.c \
../src2/platform/solidrun/hummingboard_base_pro_sdl.c \
../src2/platform/solidrun/hummingboard_gate_edge_dq.c \
../src2/platform/solidrun/hummingboard_gate_edge_sdl.c 

OBJS += \
./src2/platform/solidrun/hummingboard_base_pro_dq.o \
./src2/platform/solidrun/hummingboard_base_pro_sdl.o \
./src2/platform/solidrun/hummingboard_gate_edge_dq.o \
./src2/platform/solidrun/hummingboard_gate_edge_sdl.o 

C_DEPS += \
./src2/platform/solidrun/hummingboard_base_pro_dq.d \
./src2/platform/solidrun/hummingboard_base_pro_sdl.d \
./src2/platform/solidrun/hummingboard_gate_edge_dq.d \
./src2/platform/solidrun/hummingboard_gate_edge_sdl.d 


# Each subdirectory must supply rules for building sources it contributes
src2/platform/solidrun/%.o: ../src2/platform/solidrun/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I/usr/arm-linux-gnueabihf/include -I/home/echocare/GitRepositories/wiringX/src -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/src/Radar" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/include" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/src2" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/src2/soc/nxp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


