include $(SIMULATOR_DIR)/lv_drivers/display/display.mk
include $(SIMULATOR_DIR)/lv_drivers/indev/indev.mk


CSRCS += win_drv.c

DEPPATH += --dep-path $(SIMULATOR_DIR)/lv_drivers
VPATH += :$(SIMULATOR_DIR)/lv_drivers

CFLAGS += "-I$(SIMULATOR_DIR)/lv_drivers"
