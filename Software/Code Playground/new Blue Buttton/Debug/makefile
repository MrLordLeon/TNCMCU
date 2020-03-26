################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
new\ Blue\ Buttton.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
new\ Blue\ Buttton.list \

OBJCOPY_BIN += \
new\ Blue\ Buttton.bin \


# All Target
all: new\ Blue\ Buttton.elf secondary-outputs

# Tool invocations
new\ Blue\ Buttton.elf: $(OBJS) $(USER_OBJS) C:\Users\monke\Documents\GitHub\TNCMCU\Software\Code\ Playground\new\ Blue\ Buttton\STM32F446RETX_FLASH.ld
	arm-none-eabi-gcc -o "new Blue Buttton.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"C:\Users\monke\Documents\GitHub\TNCMCU\Software\Code Playground\new Blue Buttton\STM32F446RETX_FLASH.ld" --specs=nosys.specs -Wl,-Map="new Blue Buttton.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

new\ Blue\ Buttton.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "new Blue Buttton.list"
	@echo 'Finished building: $@'
	@echo ' '

new\ Blue\ Buttton.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "new Blue Buttton.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
