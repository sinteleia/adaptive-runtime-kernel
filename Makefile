# ARK / RTK build

BUILD_DIR := build/rtk
RTK_LIB := $(BUILD_DIR)/libark-rtk.a

TOOLCHAIN_BIN ?=

CROSS_COMPILE ?= arm-none-eabi-
TOOLCHAIN_PREFIX := $(if $(TOOLCHAIN_BIN),$(TOOLCHAIN_BIN)/,)

CC := $(TOOLCHAIN_PREFIX)$(CROSS_COMPILE)gcc
CXX := $(TOOLCHAIN_PREFIX)$(CROSS_COMPILE)g++
AS := $(TOOLCHAIN_PREFIX)$(CROSS_COMPILE)gcc
AR := $(TOOLCHAIN_PREFIX)$(CROSS_COMPILE)ar

MCU ?= cortex-m4
FPU ?= fpv4-sp-d16
FLOAT_ABI ?= hard

CSTD ?= gnu99
CXXSTD ?= gnu++14

APP_INC ?=

# Standalone check build only. Real firmware projects usually compile RTK
# directly and provide their own CMSIS/device headers.
CMSIS_INC ?= firmware/boards/NUCLEO-F446-RE/BasicDemo/Drivers/CMSIS/Include
DEVICE_INC ?= firmware/boards/NUCLEO-F446-RE/BasicDemo/Drivers/CMSIS/Device/ST/STM32F4xx/Include
DEVICE_HEADER ?= stm32f4xx.h
DEVICE_DEFINE ?= STM32F446xx

INCLUDES := \
	-Ifirmware/ark/inc \
	-Ifirmware/ark/cfg \
	-Ifirmware/mylib/inc \
	-I$(DEVICE_INC) \
	$(if $(CMSIS_INC),-I$(CMSIS_INC)) \
	$(APP_INC)

TARGET_FLAGS := \
	$(if $(DEVICE_HEADER),-include $(DEVICE_HEADER)) \
	$(if $(DEVICE_DEFINE),-D$(DEVICE_DEFINE))

CPU_FLAGS := \
	-mcpu=$(MCU) \
	-mthumb \
	-mfpu=$(FPU) \
	-mfloat-abi=$(FLOAT_ABI)

COMMON_FLAGS := \
	$(CPU_FLAGS) \
	$(INCLUDES) \
	-ffunction-sections \
	-fdata-sections \
	-Wall \
	-Wextra

CFLAGS ?= -O2
CXXFLAGS ?= -O2 -fno-exceptions -fno-rtti
ASFLAGS ?=

RTK_C_SRCS := \
	firmware/ark/src/LastErr.c \
	firmware/ark/src/MM.c \
	firmware/ark/src/RTK_Error.c \
	firmware/ark/src/RTK_Wait.c \
	firmware/ark/src/Sched.c \
	firmware/ark/src/TaskDiag.c \
	firmware/ark/src/Tic.c \
	firmware/ark/src/TimerTic.c

RTK_CPP_SRCS := \
	firmware/ark/cpp/MyNew.cpp

ifeq ($(CPP_TASK),1)
RTK_CPP_SRCS += firmware/ark/cpp/CPP_Task.cpp
endif

RTK_ASM_SRCS := \
	firmware/ark/asm/CountingSemAsm.s \
	firmware/ark/asm/SchedAsm.s \
	firmware/ark/asm/SemAsm.s

RTK_OBJS := \
	$(patsubst firmware/ark/src/%.c,$(BUILD_DIR)/src/%.o,$(RTK_C_SRCS)) \
	$(patsubst firmware/ark/cpp/%.cpp,$(BUILD_DIR)/cpp/%.o,$(RTK_CPP_SRCS)) \
	$(patsubst firmware/ark/asm/%.s,$(BUILD_DIR)/asm/%.o,$(RTK_ASM_SRCS))

.PHONY: firmware rtk clean

firmware: rtk

rtk: $(RTK_LIB)

$(RTK_LIB): $(RTK_OBJS)
	$(AR) rcs $@ $^

$(BUILD_DIR)/src/%.o: firmware/ark/src/%.c
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(CC) $(COMMON_FLAGS) $(TARGET_FLAGS) $(CFLAGS) -std=$(CSTD) -c $< -o $@

$(BUILD_DIR)/cpp/%.o: firmware/ark/cpp/%.cpp
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(CXX) $(COMMON_FLAGS) $(TARGET_FLAGS) $(CXXFLAGS) -std=$(CXXSTD) -c $< -o $@

$(BUILD_DIR)/asm/%.o: firmware/ark/asm/%.s
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(AS) $(COMMON_FLAGS) $(ASFLAGS) -x assembler-with-cpp -c $< -o $@

clean:
	@if exist "$(subst /,\,$(BUILD_DIR))" rmdir /s /q "$(subst /,\,$(BUILD_DIR))"
