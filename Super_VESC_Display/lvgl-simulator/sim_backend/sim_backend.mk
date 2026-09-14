# Fake firmware backend for the simulator (SIM_SHIM=1).
#
# Five of the seven menus in this UI are device-only: they talk to vesc_can,
# the PAS/speed tasks, the LISP store or the filesystem, so the desktop build
# compiled a "this screen is on the device build" placeholder instead. That
# hid exactly the screens with the most layout to get wrong — VESC Config is
# 1600 lines of it.
#
# With SIM_SHIM=1 the real screens compile on the host instead: the firmware's
# own headers, a few one-page shims for esp_log/esp_err/FreeRTOS, and
# sim_backend.c standing in for the hardware with plausible data. The VESC
# parameter tables and their runtime are the firmware's own sources, so the
# config menu shows the real parameter names, groups and ranges.
FW_ROOT       := $(PRJ_DIR)/..
SIM_BACKEND   := $(makefile_path)/sim_backend

CFLAGS += -DLV_SIMSHIM=1
# The LISP editor needs the keyboard widget, which the desktop lv_conf
# leaves off because no simulator screen used it before.
CFLAGS += -DLV_USE_KEYBOARD=1
# Font sizes the device screens ask for by name. The device gets these from
# the ESP-IDF lv_conf; the desktop one ships only what GUI Guider needed.
CFLAGS += -DLV_FONT_MONTSERRAT_48=1
# GUI Guider generated this project with the keyboard switched off, so the
# reference demo screen's keyboard branch refers to an lv_ui field that was
# never generated (g_kb_top_layer). Turning the keyboard on for the LISP editor
# exposes that. Point the name at a field that does exist: it is the user_data
# of a callback on a demo screen nothing here opens, and generated/ gets
# overwritten by GUI Guider, so patching the file would not stick. Only the
# 800x480 project has that screen at all.
CFLAGS += -Dg_kb_top_layer=reference
CFLAGS += "-I$(SIM_BACKEND)/shim"
CFLAGS += "-I$(SIM_BACKEND)"
CFLAGS += "-I$(FW_ROOT)/main"
CFLAGS += "-I$(FW_ROOT)/components/vesc_can/include"
CFLAGS += "-I$(FW_ROOT)/components/vesc_config/include"
# vesc_config_priv.h is the runtime<->transport boundary we fake.
CFLAGS += "-I$(FW_ROOT)/components/vesc_config"
CFLAGS += "-I$(FW_ROOT)/components/vesc_ui/include"
CFLAGS += "-I$(FW_ROOT)/components/dev_settings/include"

CSRCS += sim_backend.c
DEPPATH += --dep-path $(SIM_BACKEND)
VPATH += :$(SIM_BACKEND)

# The firmware's own parameter tables and config runtime — that is the point of
# building the real menu rather than a mock of it.
VC_DIR := $(FW_ROOT)/components/vesc_config
CSRCS += vesc_config_runtime.c vesc_config_serdes.c
CSRCS += $(notdir $(wildcard $(VC_DIR)/generated/*.c))
DEPPATH += --dep-path $(VC_DIR) --dep-path $(VC_DIR)/generated
VPATH += :$(VC_DIR):$(VC_DIR)/generated

# buffer.c is the VESC wire (de)serializer the config serdes is built on —
# pure C, so the host uses the firmware's own copy.
VESC_CAN_DIR := $(FW_ROOT)/components/vesc_can
CSRCS += buffer.c crc.c
DEPPATH += --dep-path $(VESC_CAN_DIR)
VPATH += :$(VESC_CAN_DIR)
