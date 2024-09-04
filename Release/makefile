################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src2/soc/samsung/subdir.mk
-include src2/soc/nxp/subdir.mk
-include src2/soc/broadcom/subdir.mk
-include src2/soc/amlogic/subdir.mk
-include src2/soc/allwinner/subdir.mk
-include src2/soc/subdir.mk
-include src2/platform/xunlong/subdir.mk
-include src2/platform/solidrun/subdir.mk
-include src2/platform/raspberrypi/subdir.mk
-include src2/platform/linksprite/subdir.mk
-include src2/platform/lemaker/subdir.mk
-include src2/platform/hardkernel/subdir.mk
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
all: NoveldaApp

# Tool invocations
NoveldaApp: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross G++ Linker'
	arm-linux-gnueabihf-g++ -L/usr/lib -o "NoveldaApp" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(OBJS)$(C_UPPER_DEPS)$(CXX_DEPS)$(C_DEPS)$(CPP_DEPS) NoveldaApp
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
