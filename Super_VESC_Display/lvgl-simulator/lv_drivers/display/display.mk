CSRCS += fbdev.c
CSRCS += monitor.c
CSRCS += R61581.c
CSRCS += SSD1963.c
CSRCS += ST7565.c
CSRCS += UC1610.c
CSRCS += SHARP_MIP.c
CSRCS += icon.c

DEPPATH += --dep-path $(SIMULATOR_DIR)/lv_drivers/display
VPATH += :$(SIMULATOR_DIR)/lv_drivers/display

CFLAGS += "-I$(SIMULATOR_DIR)/lv_drivers/display"
