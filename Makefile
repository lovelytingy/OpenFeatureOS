# =============================================================================
# OpenFeatureOS Makefile
# Target: Unisoc T107 (UMS9117) / ARM Cortex-A7 (Nokia 2660 Flip TA-1480)
# Language Policy: C and ARM Assembly only.
# =============================================================================

CROSS_COMPILE ?= arm-none-eabi-
CC             = $(CROSS_COMPILE)gcc
AS             = $(CROSS_COMPILE)gcc
LD             = $(CROSS_COMPILE)ld
OBJCOPY        = $(CROSS_COMPILE)objcopy
OBJDUMP        = $(CROSS_COMPILE)objdump
SIZE           = $(CROSS_COMPILE)size
PYTHON         = python3

# Target architecture flags: ARM Cortex-A7, 32-bit ARM mode, VFPv4-D16 FPU
ARCH_FLAGS     = -mcpu=cortex-a7 -marm -mfpu=vfpv4-d16 -mfloat-abi=softfp

# C compiler flags
CFLAGS         = $(ARCH_FLAGS) \
                 -std=gnu99 \
                 -Wall -Wextra -Werror=implicit-function-declaration \
                 -O2 -g \
                 -ffreestanding -fno-builtin -nostdlib \
                 -Iinclude -Iboard

# Assembly flags
AFLAGS         = $(ARCH_FLAGS) \
                 -x assembler-with-cpp \
                 -g \
                 -Iinclude -Iboard

# Linker script and flags
LDSCRIPT       = board/link.ld
LDFLAGS        = -T $(LDSCRIPT) -nostdlib -Wl,--gc-sections

# Output build directory
BUILD_DIR      = build
TARGET_ELF     = $(BUILD_DIR)/openfeatureos.elf
TARGET_RAW     = $(BUILD_DIR)/openfeatureos.raw
TARGET_BIN     = $(BUILD_DIR)/openfeatureos.bin

# Sources
C_SRCS        := $(wildcard src/*.c) $(wildcard board/*.c)
ASM_SRCS      := $(wildcard board/*.S) $(wildcard src/*.S)

OBJS          := $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SRCS)) \
                 $(patsubst %.S, $(BUILD_DIR)/%.o, $(ASM_SRCS))

# Emulator launcher
EMU_LAUNCHER   = third_party/nokia-2660-emulator/bin/n2660emu

.PHONY: all clean emu disasm help info run

all: $(TARGET_BIN)

# Ensure build directories exist
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)/board $(BUILD_DIR)/src

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "  AS      $<"
	@$(AS) $(AFLAGS) -c $< -o $@

$(TARGET_ELF): $(OBJS) $(LDSCRIPT) | $(BUILD_DIR)
	@echo "  LD      $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@
	@$(SIZE) $@

$(TARGET_RAW): $(TARGET_ELF)
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

$(TARGET_BIN): $(TARGET_RAW) scripts/mkdhtb.py
	@echo "  MKDHTB  $@"
	@$(PYTHON) scripts/mkdhtb.py $(TARGET_RAW) $(TARGET_BIN)
	@echo "[+] Build complete: $(TARGET_BIN)"

disasm: $(TARGET_ELF)
	@$(OBJDUMP) -d -S $(TARGET_ELF) > $(BUILD_DIR)/openfeatureos.disasm
	@echo "[+] Disassembly generated: $(BUILD_DIR)/openfeatureos.disasm"

emu: $(TARGET_BIN)
	@if [ -x "$(EMU_LAUNCHER)" ]; then \
		echo "[*] Launching in Nokia 2660 Flip Emulator (n2660emu)..."; \
		./$(EMU_LAUNCHER) $(TARGET_BIN) --prefer-pac --entry 0x80100200 --timeout 2; \
	else \
		echo "[-] Emulator launcher not found at $(EMU_LAUNCHER)"; \
		exit 1; \
	fi

run: $(TARGET_BIN)
	@./scripts/run_device.sh

clean:
	@rm -rf $(BUILD_DIR)
	@echo "[+] Cleaned build directory."

info:
	@echo "OpenFeatureOS Build Configuration:"
	@echo "  Toolchain:    $(CC)"
	@echo "  Arch flags:   $(ARCH_FLAGS)"
	@echo "  Linker Script:$(LDSCRIPT)"
	@echo "  C Sources:    $(C_SRCS)"
	@echo "  ASM Sources:  $(ASM_SRCS)"

help:
	@echo "OpenFeatureOS Build System"
	@echo "Targets:"
	@echo "  all      - Build openfeatureos.bin (DHTB wrapped for DRAM 0x80100000)"
	@echo "  clean    - Remove build artifacts"
	@echo "  disasm   - Generate disassembly file in build/"
	@echo "  emu      - Run payload in n2660emu"
	@echo "  info     - Display toolchain and build flags"
