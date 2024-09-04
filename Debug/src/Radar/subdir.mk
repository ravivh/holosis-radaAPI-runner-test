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
	arm-linux-gnueabihf-g++ -D__cplusplus=201103L -I/usr/arm-linux-gnueabihf/include -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/include" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/src2" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/src/XDriver" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/src/hal" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/src/Radar" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/libxml2-2.7.2/include" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/src" -I"//home/echocare/GitRepositories/Novelda Matlab Candidate/src2/soc/nxp" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


