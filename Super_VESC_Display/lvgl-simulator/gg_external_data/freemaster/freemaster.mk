
CSRCS += connect_utils.c
CSRCS += freemaster_client.c

DEPPATH += --dep-path $(SIMULATOR_DIR)/gg_external_data/freemaster
VPATH += :$(SIMULATOR_DIR)/gg_external_data/freemaster

CFLAGS += "-I$(SIMULATOR_DIR)/gg_external_data/freemaster"
