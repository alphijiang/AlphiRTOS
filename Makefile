COMPILER ?= gcc
SEMIHOSTING ?= 0
# ==========================
# OS 偵測
# ==========================
ifeq ($(OS),Windows_NT)
    # Windows cmd.exe 語法
    RM      := del /Q
    RMDIR   := rmdir /S /Q
    MKDIR   := mkdir
    SEP     := \

    # EXIST 巨集 (檢查檔案存在才執行)
    EXIST = if exist $(1) $(2)
else
    # Linux / Mac 語法
    RM      := rm -f
    RMDIR   := rm -rf
    MKDIR   := mkdir -p
    SEP     := /

    # EXIST 巨集
    EXIST = if [ -e $(1) ]; then $(2); fi
endif

TOOLCHAIN_PREFIX ?= riscv64-unknown-elf-
CC = ${TOOLCHAIN_PREFIX}$(COMPILER)
AS = ${TOOLCHAIN_PREFIX}$(COMPILER)
LD = ${TOOLCHAIN_PREFIX}$(COMPILER)
SZ = ${TOOLCHAIN_PREFIX}size
OC = ${TOOLCHAIN_PREFIX}objcopy

TARGET = firmware
ELF ?= $(TARGET).elf
BIN ?= $(TARGET).bin
HEX ?= $(TARGET).hex
MAP ?= $(TARGET).map

CFLAGS ?=  -march=rv32imac_zicsr_zifencei -mabi=ilp32 -falign-functions=16 -mstrict-align -mcmodel=medany -ffunction-sections -fdata-sections -g3 -O2
CDEFINES ?=

LDFLAGS ?= -nostartfiles -Wl,--gc-sections -Wl,-Map=$(MAP) -Wl,-Tlink.ld

CSRCS = syscall.c uart.c rtos.c main.c 
ASRCS = crt0.S kernel.S

OBJS = $(CSRCS:.c=.o) $(ASRCS:.S=.o)

ifeq ($(COMPILER),clang)
    CFLAGS+= --gcc-toolchain=/opt/riscv --target=riscv64-unknown-elf
endif

ifeq ($(SEMIHOSTING),1)
	CDEFINES+= -D__SEMIHOSTING=1
endif

ifeq ($(FPGA),1)
	CDEFINES+= -DFPGA
endif

ifeq ($(COMPILER),clang)
    LDFLAGS+= -lnosys -lc_nano -lm_nano
    ifeq ($(SEMIHOSTING),1)
    	LDFLAGS+= -lsemihost
    endif
else
    LDFLAGS+= --specs=nano.specs --specs=nosys.specs
    ifeq ($(SEMIHOSTING),1)
    	LDFLAGS+= --specs=semihost.specs
    endif
endif

all: elf
	@$(call EXIST,$(ELF),$(SZ) $(ELF))
	@$(RM) $(OBJS)
%.o: %.c
	$(CC) $(CFLAGS) $(CDEFINES) -c $< -o $@

%.o: %.S
	$(AS) $(CFLAGS) $(CDEFINES) -c $< -o $@

elf: $(OBJS)
	$(LD) $(CFLAGS) $(LDFLAGS) -o $(ELF) $(OBJS)

bin: elf
	@$(call EXIST,$(ELF),$(OC) -O binary $(ELF) $(BIN))

hex: elf
	@$(call EXIST,$(ELF),$(OC) -O ihex $(ELF) $(HEX))

prog:
ifeq ($(OS),Windows_NT)
	@$(call EXIST,$(ELF),$(TOOLCHAIN_PREFIX)gdb -ex "set $$mode=0" -x ./gdb.scr $(ELF))
else
	@$(call EXIST,$(ELF),$(TOOLCHAIN_PREFIX)gdb -ex 'set $$mode=0' -x ./gdb.scr $(ELF))
endif

debug:
ifeq ($(OS),Windows_NT)
	@$(call EXIST,$(ELF),$(TOOLCHAIN_PREFIX)gdb -ex "set $$mode=1" -x ./gdb.scr $(ELF))
else
	@$(call EXIST,$(ELF),$(TOOLCHAIN_PREFIX)gdb -ex 'set $$mode=1' -x ./gdb.scr $(ELF))
endif

clean:
	-$(RM) $(ELF) $(BIN) $(HEX) $(MAP) $(OBJS)
