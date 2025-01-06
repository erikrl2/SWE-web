THISDIR := $(dir $(lastword $(MAKEFILE_LIST)))

# Set by cmake target
PLATFORM ?=
PROFILE ?=
BGFX_DIR ?= $(THISDIR)../Build/_deps/bgfx-src/bgfx/

SHADER_DIR := $(THISDIR)../Assets/Shaders/

SHADERC := $(THISDIR)../Tools/Bin/shaderc

ifeq ($(OS),Windows_NT)
    SHADERC := $(SHADERC)-win64
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
        SHADERC := $(SHADERC)-macos
	else
        SHADERC := $(SHADERC)-linux64
	endif
endif

VS_FLAGS += -i $(BGFX_DIR)src/ -i $(SHADER_DIR)common/ --type vertex
FS_FLAGS += -i $(BGFX_DIR)src/ -i $(SHADER_DIR)common/ --type fragment

VS_SOURCES = $(shell find $(SHADER_DIR) -type f -name "vs_*.sc")
FS_SOURCES = $(shell find $(SHADER_DIR) -type f -name "fs_*.sc")

VS_BIN = $(addsuffix .bin.h, $(basename $(VS_SOURCES)))
FS_BIN = $(addsuffix .bin.h, $(basename $(FS_SOURCES)))

BIN = $(VS_BIN) $(FS_BIN)

# ----------------------------------------------------------------------------
vs_%.bin.h : vs_%.sc
#	@echo "[$<]"
	@$(SHADERC) $(VS_FLAGS) \
		--platform $(PLATFORM) --profile $(PROFILE) \
		-f "$<" -o "$@" \
		--bin2c $(basename $(notdir $<))

# ----------------------------------------------------------------------------
fs_%.bin.h : fs_%.sc
#	@echo "[$<]"
	@$(SHADERC) $(FS_FLAGS) \
		--platform $(PLATFORM) --profile $(PROFILE) \
		-f "$<" -o "$@" \
		--bin2c $(basename $(notdir $<))

# ----------------------------------------------------------------------------
.PHONY: all clean rebuild

all: $(BIN)

clean:
	@-rm -f $(BIN)

rebuild: clean all
