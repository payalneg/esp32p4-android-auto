-include $(PRJ_DIR)/generated/images/images_list.mk
-include $(PRJ_DIR)/generated/images/lottie_list.mk

DEPPATH += --dep-path $(PRJ_DIR)/generated/images
VPATH += :$(PRJ_DIR)/generated/images

CFLAGS += "-I$(PRJ_DIR)/generated/images"