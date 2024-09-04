################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src2/soc/nxp/subdir.mk
-include src2/soc/subdir.mk
-include src2/platform/solidrun/subdir.mk
-include src2/platform/subdir.mk
-include src2/subdir.mk
-include src/hal/subdir.mk
-include src/XDriver/subdir.mk
-include src/Radar/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: NoveldaAppMatlab_01.04

# Tool invocations
NoveldaAppMatlab_01.04: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross G++ Linker'
	arm-linux-gnueabihf-g++ -L/home/echocare/GitRepositories/WiringPi-master/WiringPi-master/wiringPi -L/usr/lib -L"//home/echocare/GitRepositories/Novelda Matlab Candidate/libxml2-2.7.2/lib" -o "NoveldaAppMatlab_01.04" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(OBJS)$(C_UPPER_DEPS)$(CXX_DEPS)$(C_DEPS)$(CPP_DEPS) NoveldaAppMatlab_01.04
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
