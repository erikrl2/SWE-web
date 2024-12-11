SILENT?=@

THISDIR:=$(dir $(lastword $(MAKEFILE_LIST)))
SHADERC:="$(THISDIR)../Tools/Bin/shaderc-linux64"

# Paths are correctly set by cmake target when building
BGFX_DIR?=$(THISDIR)../Build/_deps/bgfx-src/bgfx/
SHADER_DIR?=$(THISDIR)../Assets/Shaders/

VS_FLAGS+=-i $(BGFX_DIR)src/ -i $(SHADER_DIR)common/ --type vertex
FS_FLAGS+=-i $(BGFX_DIR)src/ -i $(SHADER_DIR)common/ --type fragment

VS_SOURCES=$(shell find $(SHADER_DIR) -type f -name "vs_*.sc")
FS_SOURCES=$(shell find $(SHADER_DIR) -type f -name "fs_*.sc")

VS_BIN=$(addsuffix .bin.h, $(basename $(VS_SOURCES)))
FS_BIN=$(addsuffix .bin.h, $(basename $(FS_SOURCES)))

BIN=$(VS_BIN) $(FS_BIN)

SHADER_TMP=tmp

vs_%.bin.h : vs_%.sc
	@echo [$(<)]
	 $(SILENT) $(SHADERC) $(VS_FLAGS) --platform linux   -p 120    -f $(<) -o "$(SHADER_TMP)" --bin2c $(shell basename $(<) .sc)_glsl
	@cat "$(SHADER_TMP)" > $(@)
	-$(SILENT) $(SHADERC) $(VS_FLAGS) --platform asm.js  -p 100_es -f $(<) -o "$(SHADER_TMP)" --bin2c $(basename $(notdir $(<)))_essl
	-@cat "$(SHADER_TMP)" >> $(@)
	-@printf "static const uint8_t $(basename $(notdir $(<)))_spv[]{0}; // not supported\n" | tr -d '\015' >> $(@)
	-@printf "static const uint8_t $(basename $(notdir $(<)))_dx11[]{0}; // not supported\n" | tr -d '\015' >> $(@)
	-@printf "static const uint8_t $(basename $(notdir $(<)))_mtl[]{0}; // not supported\n" | tr -d '\015' >> $(@)

fs_%.bin.h : fs_%.sc
	@echo [$(<)]
	 $(SILENT) $(SHADERC) $(FS_FLAGS) --platform linux   -p 120   -f $(<) -o "$(SHADER_TMP)" --bin2c $(basename $(notdir $(<)))_glsl
	@cat "$(SHADER_TMP)" > $(@)
	-$(SILENT) $(SHADERC) $(FS_FLAGS) --platform asm.js -p 100_es -f $(<) -o "$(SHADER_TMP)" --bin2c $(basename $(notdir $(<)))_essl
	-@cat "$(SHADER_TMP)" >> $(@)
	-@printf "static const uint8_t $(basename $(notdir $(<)))_spv[]{0}; // not supported\n" | tr -d '\015' >> $(@)
	-@printf "static const uint8_t $(basename $(notdir $(<)))_dx11[]{0}; // not supported\n" | tr -d '\015' >> $(@)
	-@printf "static const uint8_t $(basename $(notdir $(<)))_mtl[]{0}; // not supported\n" | tr -d '\015' >> $(@)

.PHONY: all
all: $(BIN)
	@-rm -f $(SHADER_TMP)

.PHONY: clean
clean:
	@echo Cleaning...
	@-rm -vf $(BIN)

.PHONY: rebuild
rebuild: clean all
