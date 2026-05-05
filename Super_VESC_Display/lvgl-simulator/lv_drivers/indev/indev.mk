CSRCS += FT5406EE8.c
CSRCS += keyboard.c
CSRCS += mouse.c
CSRCS += mousewheel.c
CSRCS += evdev.c
CSRCS += libinput.c
CSRCS += XPT2046.c

DEPPATH += --dep-path $(SIMULATOR_DIR)/lv_drivers/indev
VPATH += :$(SIMULATOR_DIR)/lv_drivers/indev

CFLAGS += "-I$(SIMULATOR_DIR)/lv_drivers/indev"
