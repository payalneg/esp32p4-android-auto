# SPDX-License-Identifier: MIT
# Copyright 2020-2023 NXP

makefile_name := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_path := $(strip $(patsubst %/,% , $(dir $(makefile_name))))
libfile_path := $(makefile_path)/../lib/native

CC := gcc
PRJ_DIR := $(makefile_path)/..
LVGL_DIR := $(PRJ_DIR)
SIMULATOR_DIR := $(makefile_path)
LVGL_DIR_NAME ?= lvgl
CFLAGS := -O3 -g0 -I$(LVGL_DIR)/ -I$(makefile_path)/ -I$(makefile_path)/rlottie -c -fmessage-length=0
CFLAGS += -I$(LVGL_DIR)/$(LVGL_DIR_NAME)/ -I$(LVGL_DIR)/$(LVGL_DIR_NAME)/src

UNAME_S := $(shell uname -s)

ifeq ($(OS),Windows_NT)
-include $(SIMULATOR_DIR)/gg_external_data/freemaster/freemaster.mk
-include $(SIMULATOR_DIR)/gg_external_data/gg_external_data.mk
LDFLAGS := -lSDL2 -L$(libfile_path) -ldecoder -lopenh264 -lrlottie -lpthread -lstdc++ -ljansson -lcurl -lm
else ifeq ($(UNAME_S),Darwin)
BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
ifeq ($(BREW_PREFIX),)
BREW_PREFIX := /opt/homebrew
endif
CFLAGS  += -I$(BREW_PREFIX)/include
LDFLAGS := -L$(BREW_PREFIX)/lib -lSDL2 -L$(libfile_path) -ldecoder -lopenh264 -lrlottie -lpthread -lstdc++ -lm
else
LDFLAGS := -lSDL2 -L$(libfile_path) -ldecoder -lopenh264 -lrlottie -lpthread -lstdc++ -lm -ldl
endif

BUILD_DIR := build/
OBJ_DIR := $(BUILD_DIR)object/
GEN_OBJ_DIR := $(OBJ_DIR)generated/
BIN_DIR := $(BUILD_DIR)bin/

NORMAL_OUT := 1

RM := rm -f
RMDIR := rm -rf

ifeq ($(OS),Windows_NT)
LDFLAGS += -LSDL2\i686-w64-mingw32\lib
SIMULATOR_EXE = simulator.exe
SIMULATOR_DLL = simulator.dll
BUILD_ENV := build\object build\bin
NULL_OUT := nul
MKDIR_P = @if not exist "$(1)" mkdir "$(1)"
else
SIMULATOR_EXE = simulator
NULL_OUT := /dev/null
MKDIR_P = @if [ ! -d "$(1)" ]; then mkdir -p "$(1)"; fi
endif

CFLAGS += -DLV_CONF_INCLUDE_SIMPLE=1 $(EXTRA_CFLAGS)

#Collect the files to compile
-include $(LVGL_DIR)/$(LVGL_DIR_NAME)/lvgl.mk
-include $(SIMULATOR_DIR)/lv_drivers/lv_drivers.mk
# generated.mk pulls in the shared fonts and images plus this project's own
# screens. SCREENS_DIR swaps the screens for another project's — the 480x480
# one bootstrapped by tools/make_480_project.py — while fonts, images, custom/
# and lvgl stay shared. `make dash480` below sets it together with the matching
# resolution.
-include $(PRJ_DIR)/generated/generated.mk
SCREENS_DIR ?= $(PRJ_DIR)
ifneq ($(SCREENS_DIR),$(PRJ_DIR))
GEN_CSRCS := $(filter-out $(notdir $(wildcard $(PRJ_DIR)/generated/*.c)),$(GEN_CSRCS))
GEN_CSRCS += $(notdir $(wildcard $(SCREENS_DIR)/generated/*.c))
DEPPATH += --dep-path $(SCREENS_DIR)/generated
VPATH := $(SCREENS_DIR)/generated:$(VPATH)
# Prepended, not appended: gui_guider.h must resolve to this project's copy.
CFLAGS := "-I$(SCREENS_DIR)/generated" $(CFLAGS)
endif
-include $(PRJ_DIR)/custom/custom.mk

CSRCS += main.c

# Auto-registered dashboard themes. scripts/gen_dashboard_themes.py turns every
# convention-named dashboard_* screen in generated/gui_guider.h into a generic
# theme (custom/theme_generic.c) so it shows up in the Settings theme dropdown.
# The ESP-IDF build does the same in components/vesc_ui/CMakeLists.txt; without
# this the simulator would only ever offer the hand-written cockpit theme.
# A real file target (not a .PHONY step) so -j cannot race the compile. The rule
# itself lives further down, below `all:` — the first target in a makefile is the
# default goal, and a rule here would hijack it.
THEMES_AUTO := $(makefile_path)/dashboard_themes_auto.c
CSRCS += dashboard_themes_auto.c

DEPPATH += --dep-path $(makefile_path)/
VPATH += :$(makefile_path)/
CFLAGS += "-I$(makefile_path)/"

ifeq ($(OS),Windows_NT)
#SDL2
CFLAGS += "-ISDL2\i686-w64-mingw32\include" -MMD -MP
endif

OBJEXT := .o

AOBJS = $(addprefix $(OBJ_DIR),$(ASRCS:.S=$(OBJEXT)))
COBJS = $(addprefix $(OBJ_DIR),$(CSRCS:.c=$(OBJEXT)))
DEPS = $(addprefix $(OBJ_DIR),$(CSRCS:.c=.d))
GEN_AOBJS = $(addprefix $(GEN_OBJ_DIR),$(GEN_ASRCS:.S=$(OBJEXT)))
GEN_COBJS = $(addprefix $(GEN_OBJ_DIR),$(GEN_CSRCS:.c=$(OBJEXT)))

DEPS += $(addprefix $(GEN_OBJ_DIR),$(GEN_CSRCS:.c=.d))


SRCS = $(ASRCS) $(CSRCS)
OBJS = $(AOBJS) $(COBJS)
GEN_SRCS = $(GEN_ASRCS) $(GEN_CSRCS)
GEN_OBJS = $(GEN_AOBJS) $(GEN_COBJS)

.PHONY: all env clean default cp_lib clean_gen run regen_fonts dash480
ifeq ($(OS),Windows_NT)
all: env regen_fonts default cp_lib
else
all: env regen_fonts default
endif
# monitor header files
-include $(DEPS)

# Prepare GUI Guider artefacts for Cockpit:
#  1) regen_cockpit_fonts.py - trim Antonio_Regular@{220,64,32} to the range
#     we actually need (otherwise Antonio@220 is ~10MB and bitmap_index
#     overflows the LVGL bitfield).
#  2) patch_cockpit_dashboard.py - inject calls into setup_scr_dashboard.c
#     that hide the blue textarea cursor and disable widget dragging.
# Both scripts are idempotent.
regen_fonts:
	@python3 $(PRJ_DIR)/tools/regen_cockpit_fonts.py
	@python3 $(PRJ_DIR)/tools/patch_cockpit_dashboard.py

GENTHEME_TOOL := $(PRJ_DIR)/../scripts/gen_dashboard_themes.py
# Depends on the generator too, matching the firmware's add_custom_command —
# editing the naming convention has to regenerate even when no screen changed.
$(THEMES_AUTO): $(SCREENS_DIR)/generated/gui_guider.h $(GENTHEME_TOOL)
	@python3 $(GENTHEME_TOOL) --header $< --out $@

# CSRCS names the file relatively, so the pattern rule builds a target make does
# not identify with the absolute path above — without this the registrar would
# be generated once and then never refreshed when a screen is added or removed.
$(OBJ_DIR)dashboard_themes_auto.o: $(THEMES_AUTO)

# Build the square 480x480 dashboard instead of the 800x480 one: the ESP32-S3
# board's screens, in a window the same shape as its panel. Separate build dir
# so the two geometries don't overwrite each other's objects.
#   make dash480 && ./build480/bin/simulator
dash480:
	@$(MAKE) -f $(makefile_name) \
	    SCREENS_DIR=$(PRJ_DIR)/../Super_VESC_Display_480 \
	    BUILD_DIR=build480/ \
	    EXTRA_CFLAGS='-DLV_HOR_RES_MAX=480 -DLV_VER_RES_MAX=480'

env:
	$(call MKDIR_P,$(BIN_DIR))
	$(call MKDIR_P,$(GEN_OBJ_DIR))

$(OBJ_DIR)main.o: main.c
	@echo "Compiling $(makefile_path)/$<"
	@$(CC)  $(CFLAGS) -c $< -o $@

$(OBJ_DIR)%.o: %.c
	@echo "Compiling $<"
	@$(CC)  $(CFLAGS) -c $< -o $@

$(GEN_OBJ_DIR)%.o: %.c
	@echo "Compiling $<"
	$(call MKDIR_P,$(dir $@))
	@$(CC)  $(CFLAGS) -c $< -o $@

default: $(OBJS) $(GEN_OBJS)
	@echo "Linking $(SIMULATOR_EXE)"
	@ar -csr $(GEN_OBJ_DIR)/libgenerated.a $(GEN_OBJS)
	@$(CC) -o $(BIN_DIR)$(SIMULATOR_EXE) $(OBJS) -L$(GEN_OBJ_DIR) -lgenerated $(LDFLAGS)
ifeq ($(OS),Windows_NT)
	@echo "Linking $(SIMULATOR_DLL)"
	@$(CC) --shared -o $(BIN_DIR)$(SIMULATOR_DLL) $(OBJS) -L$(GEN_OBJ_DIR) -lgenerated $(LDFLAGS) --entry=_DllMainCRTStartup@12
endif

cp_lib:
	-@cp SDL2/lib/SDL2.dll build/bin/SDL2.dll $(NORMAL_OUT)>$(NULL_OUT)
	-@cp multi_thread/libgcc_s_dw2-1.dll build/bin/libgcc_s_dw2-1.dll $(NORMAL_OUT)>$(NULL_OUT)
	-@cp multi_thread/pthreadGC-3.dll build/bin/pthreadGC-3.dll $(NORMAL_OUT)>$(NULL_OUT)
	-@cp multi_thread/libjansson-4.dll build/bin/libjansson-4.dll $(NORMAL_OUT)>$(NULL_OUT)
	-@cp multi_thread/libcurl.dll build/bin/libcurl.dll $(NORMAL_OUT)>$(NULL_OUT)
	-@cp SDL2/lib/libopenh264.dll build/bin/libopenh264.dll $(NORMAL_OUT)>$(NULL_OUT)

clean:
	-@$(RMDIR) build

gen_images := $(wildcard $(PRJ_DIR)/generated/images/*.c)
gen_srcs := $(gen_images) $(wildcard $(PRJ_DIR)/generated/*.c) $(wildcard $(PRJ_DIR)/generated/*.h)

ifeq ($(OS),Windows_NT)
clean_gen_img:
	-@rm $(gen_images)

clean_gen: clean_gen_img
	-@rm $(gen_srcs)
run:
	build\bin\simulator.exe
else

clean_gen_img:
	$(RM) $(LVGL_DIR)/generated/images/*.c

clean_gen: clean_gen_img
	$(RM) $(LVGL_DIR)/generated/*.c
	$(RM) $(LVGL_DIR)/generated/*.h

run:
	./build/bin/simulator
endif
