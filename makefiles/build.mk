SHELL := /usr/bin/env bash
.DEFAULT_GOAL := all

TARGET ?= msdos
FUJINET_NIO_LIB ?= ../fujinet-nio-lib
# cc65 checkout; BBC assembly includes its libsrc/bbc headers (oslib/os.inc).
CC65_HOME ?= ../cc65

include makefiles/targets.mk

# ---------------------------------------------------------------------------------------------
# Where things live (see docs/ARCHITECTURE.md)
#
#   examples/<device>/<example>/<program>.c   portable C against fujinet-nio-lib: "how to" programs
#   diagnostics/<platform>/*.c                tools that are about one machine (built for that target only)
#   diagnostics/amiga/wb2/*.c                 Amiga diagnostics that need Workbench 2 or later
#   platform/include/nio_example.h            the few machine services an example may use
#   platform/<platform>/*.c                   their implementation for each machine, compiled in at build time
#
# Every program is one .c file and the program is named after it. All programs of a target land in
# build/<target>/bin/ (flat), whatever folder they came from, so names must be unique.
# ---------------------------------------------------------------------------------------------
SUPPORT_INCLUDE_DIR := platform/include
NIO_INCLUDE_DIR := $(FUJINET_NIO_LIB)/include
BUILD_DIR ?= build
TARGET_BUILD_DIR ?= $(BUILD_DIR)/$(TARGET)

# The repository Makefile is the public Amiga entry point.  It supplies a
# named Workbench output directory; refusing this legacy default prevents an
# incompatible binary from silently reappearing in build/amiga/bin.
ifeq ($(TARGET),amiga)
ifeq ($(TARGET_BUILD_DIR),$(BUILD_DIR)/amiga)
$(error Amiga builds require TARGET_BUILD_DIR=build/amiga/wb13, wb31, or wb32; use `make amiga`)
endif
endif
OBJ_DIR := $(TARGET_BUILD_DIR)/obj
BIN_DIR := $(TARGET_BUILD_DIR)/bin
DISK_DIR := $(TARGET_BUILD_DIR)/disk

# Examples: examples/<device>/<example>/<program>.c, unless the example's folder has a NOT_ON file
# naming the targets (space separated, as in `make TARGET=...`) it cannot be built for, usually
# because the machine has too little memory for it.
EXAMPLE_ALL_SRCS := $(sort $(wildcard examples/*/*/*.c))
example_targets_excluded = $(shell cat $(dir $(1))NOT_ON 2>/dev/null)
EXAMPLE_SRCS := $(foreach src,$(EXAMPLE_ALL_SRCS),$(if $(filter $(TARGET),$(call example_targets_excluded,$(src))),,$(src)))

# Diagnostics for this machine. The Amiga ones that use Workbench 2+ DOS APIs are in wb2/ and are
# left out of the Workbench 1.3 profile (which has its own static-MountList coverage).
DIAG_SRCS := $(sort $(wildcard diagnostics/$(PLATFORM)/*.c))
ifeq ($(PLATFORM),amiga)
ifneq ($(AMIGA_WB13),1)
DIAG_SRCS += $(sort $(wildcard diagnostics/amiga/wb2/*.c))
endif
endif
# MS-DOS diagnostics share a small probe library of their own.
DIAG_LIB_SRCS := $(if $(filter msdos,$(PLATFORM)),diagnostics/msdos/lib/nio.c)
NO_NIO_LIB_PROGRAMS := irqmon     # does not use fujinet-nio-lib at all
NO_DIAG_LIB_PROGRAMS := irqmon    # nor the MS-DOS probe library
ASM_SRCS := $(sort $(wildcard diagnostics/$(PLATFORM)/*.s))
EXTRA_OBJS_streamtest := $(OBJ_DIR)/diagnostics/atari/stream_fx.o
EXTRA_OBJS_checksumbench := $(OBJ_DIR)/diagnostics/amiga/checksumbench_asm.o

# What an example may use from the machine (platform/include/nio_example.h). Nothing is linked into a
# program that does not use it: an example that includes nio_example.h gets the machine's
# nio_example.c, and one that also calls nio_hint_*() gets the hardware-hint file too (the machine's
# own nio_hint.c, or the default that just sleeps). On a small machine every byte counts.
SUPPORT_CORE_SRCS := platform/$(PLATFORM)/nio_example.c
ifeq ($(wildcard platform/$(PLATFORM)/nio_hint.c),)
SUPPORT_HINT_SRCS := platform/common/nio_hint_poll.c
else
SUPPORT_HINT_SRCS := platform/$(PLATFORM)/nio_hint.c
endif
uses_support = $(if $(shell grep -lE 'nio_(sleep_ms|key_pressed|exit_pause|hint_)' $(1) 2>/dev/null),1)
uses_hint = $(if $(shell grep -l 'nio_hint_' $(1) 2>/dev/null),1)
support_objs = $(if $(call uses_support,$(1)),$(SUPPORT_CORE_OBJS) $(if $(call uses_hint,$(1)),$(SUPPORT_HINT_OBJS)))

EXAMPLE_PROGRAMS := $(basename $(notdir $(EXAMPLE_SRCS)))
DIAG_PROGRAMS := $(basename $(notdir $(DIAG_SRCS)))
PROGRAMS := $(EXAMPLE_PROGRAMS) $(DIAG_PROGRAMS)
DUPLICATES := $(sort $(foreach p,$(PROGRAMS),$(if $(filter-out 1,$(words $(filter $(p),$(PROGRAMS)))),$(p))))
ifneq ($(DUPLICATES),)
$(error Program names must be unique across examples/ and diagnostics/: $(DUPLICATES))
endif

SUPPORT_CORE_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SUPPORT_CORE_SRCS))
SUPPORT_HINT_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SUPPORT_HINT_SRCS))
SUPPORT_OBJS := $(SUPPORT_CORE_OBJS) $(SUPPORT_HINT_OBJS)
DIAG_LIB_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(DIAG_LIB_SRCS))
EXAMPLE_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(EXAMPLE_SRCS))
DIAG_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(DIAG_SRCS))
ASM_OBJS := $(patsubst %.s,$(OBJ_DIR)/%.o,$(ASM_SRCS))
PROGRAM_BINS := $(PROGRAMS:%=$(BIN_DIR)/%$(PROGRAM_EXT))
DEPENDS := $(SUPPORT_OBJS:.o=.d) $(DIAG_LIB_OBJS:.o=.d) $(EXAMPLE_OBJS:.o=.d) $(DIAG_OBJS:.o=.d) $(ASM_OBJS:.o=.d)

ifeq ($(COMPILER_FAMILY),wcc)
include makefiles/compiler-wcc.mk
else ifeq ($(COMPILER_FAMILY),cc65)
include makefiles/compiler-cc65.mk
else ifeq ($(COMPILER_FAMILY),gcc)
include makefiles/compiler-gcc.mk
else ifeq ($(COMPILER_FAMILY),amigagcc)
include makefiles/compiler-amigagcc.mk
else
$(error Unknown compiler family '$(COMPILER_FAMILY)' for TARGET=$(TARGET))
endif

# Test-server addresses baked into the examples (see platform/include/nio_example.h).
ifneq ($(HTTPBIN_URL),)
CFLAGS += -DFN_DEFAULT_HTTPBIN_URL=\"$(HTTPBIN_URL)\"
endif
ifneq ($(TCP_URL),)
CFLAGS += -DFN_DEFAULT_TCP_URL=\"$(TCP_URL)\"
endif

DISK_TARGETS :=
-include makefiles/disk-$(TARGET).mk

.PHONY: all clean disk $(PROGRAMS) $(DISK_TARGETS)
.SECONDARY: $(EXAMPLE_OBJS) $(DIAG_OBJS) $(SUPPORT_OBJS) $(DIAG_LIB_OBJS)

all: $(PROGRAM_BINS)

disk: $(DISK_TARGETS)

$(PROGRAMS): %: $(BIN_DIR)/%$(PROGRAM_EXT)

-include $(DEPENDS)

$(NIO_LIB_FILE):
	$(MAKE) -C $(FUJINET_NIO_LIB) $(NIO_LIB_TARGET)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(call compile_c)

ifeq ($(COMPILER_FAMILY),amigagcc)
$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	m68k-amigaos-as -mcpu=68000 -o $@ $<
else
$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	ca65 -t $(TARGET) $(ASMFLAGS) -I $(CC65_HOME)/libsrc/bbc -o $@ $<
endif

# $(1) program, $(2) its object, $(3) objects it links besides the library
define PROGRAM_RULE
$(BIN_DIR)/$(1)$(PROGRAM_EXT): $(2) $(3) $$(EXTRA_OBJS_$(1)) $$(if $$(filter $(1),$$(NO_NIO_LIB_PROGRAMS)),,$$(NIO_LIB_FILE)) | $(BIN_DIR)
	$$(call link_program)
endef

$(foreach src,$(EXAMPLE_SRCS),$(eval $(call PROGRAM_RULE,$(basename $(notdir $(src))),$(OBJ_DIR)/$(basename $(src)).o,$(call support_objs,$(src)))))
$(foreach src,$(DIAG_SRCS),$(eval $(call PROGRAM_RULE,$(basename $(notdir $(src))),$(OBJ_DIR)/$(basename $(src)).o,$(if $(filter $(basename $(notdir $(src))),$(NO_DIAG_LIB_PROGRAMS)),,$(DIAG_LIB_OBJS)))))

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(DISK_DIR):
	mkdir -p $@

clean:
	rm -rf $(TARGET_BUILD_DIR)
