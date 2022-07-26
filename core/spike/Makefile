# Makefile for compiling LF programs for the risc-v spike platform
# Usage ("project" stands in for the project name):
#   - copy this Makefile from project/core/spike/ to project/
#   - run "make" in the project/ directory.
#   - verify that demo.riscv was created.
#   - To run the executable, run "spike --isa=rv32i demo.riscv"

XLEN ?= 64


src_dir = ./core/spike/
support_file = ./core/platform/lf_spike_support.c
other_files = $(wildcard ./lib/*.c)

RISCV_PREFIX ?= riscv$(XLEN)-unknown-elf-
RISCV_GCC ?= $(RISCV_PREFIX)gcc

RISCV_GCC_OPTS ?= \
				-DPREALLOCATE=1 -mcmodel=medany -static \
				-march=rv32i -mabi=ilp32 \
				-std=gnu99 \
				-Og \
				-ffast-math -fno-common -fno-builtin-printf

RISCV_LINK ?= $(RISCV_GCC) -T $(src_dir)common/link.ld $(incs)
RISCV_LINK_OPTS ?= -g -static -nostartfiles -lm -lgcc -T $(src_dir)common/link.ld

RISCV_OBJDUMP ?= $(RISCV_PREFIX)objdump --disassemble-all --disassemble-zeroes --section=.text --section=.text.startup --section=.text.init --section=.data

incs  += -I $(src_dir)common

demo.riscv: $(wildcard ./*.c) $(support_file) $(other_files) $(wildcard $(src_dir)common/*.c) $(wildcard $(src_dir)common/*.S)
	$(RISCV_GCC) $(incs) $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS) -o $@ $^

demo.dump: demo.riscv
	$(RISCV_OBJDUMP) $< > $@

demo.dump.annot: demo.riscv
	$(RISCV_OBJDUMP) -S $< > $@

all: demo.riscv demo.dump demo.dump.annot

clean:
	-rm -rf *.riscv *.dump *.dump.annot



