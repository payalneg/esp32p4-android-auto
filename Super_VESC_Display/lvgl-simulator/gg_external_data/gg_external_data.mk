
CSRCS += gg_external_data.c

DEPPATH += --dep-path $(SIMULATOR_DIR)/gg_external_data
VPATH += :$(SIMULATOR_DIR)/gg_external_data

CFLAGS += "-I$(SIMULATOR_DIR)/gg_external_data"
