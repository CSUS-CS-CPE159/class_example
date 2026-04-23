#------------------------------------------------------------------------------
# CPE/CSC 159 SPEDE3 Project Makefile
# California State University, Sacramento
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# (1) Name your operating system.  Must be a legal filename, and not contain
#     spaces or punctuation.  It will be used to name you DLI file.
#
#     Can be overridden via an environment variable, such as:
#        OS_NAME=SpedeOS make
#------------------------------------------------------------------------------
OS_NAME ?= MyOS

#------------------------------------------------------------------------------
# (2) Specify additional compiler or linker flags.
#     EXTRA_CFLAGS          Additional flags to pass to the compiler
#     EXTRA_LDFLAGS         Additional flags to pass to the linker
#------------------------------------------------------------------------------
EXTRA_CFLAGS = -Wall \
			   -Werror \
			   -Wclobbered \
			   -Wnull-dereference \
			   -Wold-style-declaration \
			   -Wsign-compare \
			   -Wtype-limits \
			   -Wuninitialized \
			   -Wunused-but-set-parameter \
			   -fdelete-null-pointer-checks

EXTRA_LDFLAGS =

#==============================================================================
# Do not modify below
#==============================================================================

#------------------------------------------------------------------------------
# General definitions
#------------------------------------------------------------------------------

SPEDE_ROOT ?= /opt/spede

# DLI filename
DLI = $(OS_NAME).dli

# Global paths
BUILD_DIR=build
SRC_DIR=src
INC = -Iinclude -I$(SRC_DIR) -I$(SPEDE_ROOT)/include/

# Compilers
CC := $(SPEDE_ROOT)/bin/i386-elf-gcc
AS := $(SPEDE_ROOT)/bin/i386-elf-as
AR := $(SPEDE_ROOT)/bin/i386-elf-ar
NM := $(SPEDE_ROOT)/bin/i386-elf-nm

# Object utilities
OBJ_COPY  := $(SPEDE_ROOT)/bin/i386-elf-objcopy
OBJ_STRIP := $(SPEDE_ROOT)/bin/i386-elf-strip
OBJ_DUMP  := $(SPEDE_ROOT)/bin/i386-elf-objdump

# Build utilities
CMD_LINKER = $(SPEDE_ROOT)/bin/linkdli
CMD_DELETE = rm -rf

# Files to be removed when a 'clean' is performed
CLEAN_FILES = $(BUILD_DIR)

# Compiler flags
ASFLAGS +=
CFLAGS  += -g -m32 -nostartfiles -nostdlib -ffreestanding -lc -DOS_NAME=\"$(OS_NAME)\" $(EXTRA_CFLAGS)
LDFLAGS += -g $(EXTRA_LDFLAGS)

src_to_bin_dir = $(patsubst $(SRC_DIR)%,$(BUILD_DIR)%,$1)

sources  = $(wildcard src/*/*.c) $(wildcard src/*.c) $(wildcard src/*.S)
objects  = $(call src_to_bin_dir,$(addsuffix .o,$(basename $(sources))))
depends  = $(patsubst %.o,%.d,$(objects))

#------------------------------------------------------------------------------
# Make targets
#------------------------------------------------------------------------------
.PHONY: $(OS_NAME) all clean debug run strip text help

all: $(DLI)
$(OS_NAME): $(DLI)

$(DLI): $(objects)
	@$(CMD_LINKER) $(LDFLAGS) -o $(BUILD_DIR)/$(DLI) $(objects)

clean:
	@echo "Removing compiled objects and images"
	@$(CMD_DELETE) $(CLEAN_FILES)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(INC) -c -o $@ $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(@D)
	@$(CC) -DASSEMBLER $(CFLAGS) $(INC) -c -o $@ $<

run: $(DLI)
	@spede-run $(BUILD_DIR)/$(DLI)

debug: $(DLI)
	@spede-run -d $(BUILD_DIR)/$(DLI)

strip: $(DLI)
	@$(OBJ_STRIP) $(BUILD_DIR)/$(DLI)
	@echo "Stripped debug symbols from $(BUILD_DIR)/$(DLI)"

text: $(DLI)
	@$(OBJ_DUMP) --disassemble --file-headers --reloc --source $(BUILD_DIR)/$(DLI) > $(BUILD_DIR)/$(DLI).asm
	@echo "Image disassembly into $(DLI).asm done"

tags:
	@ctags -R --languages=C,C++,ASM -f .tags

help:
	@echo "This Makefile builds $(DLI)."
	@echo "  make all       -- Builds an operating system image"
	@echo "  make clean     -- Remove all compiled objects and images"
	@echo "  make run       -- Runs the operating system image"
	@echo "  make debug		-- Runs the operating system image with GDB"
	@echo "  make strip     -- Builds an image with no debug symbols included"
	@echo "  make text      -- Generate annotated assembly source for the operating system image"
	@echo "  make tag       -- Generate ctags file"
	@echo ""

