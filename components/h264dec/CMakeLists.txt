# In-tree H.264 baseline decoder for the AA video pipeline.
#
# h264bsd/ is Hantro's OpenCORE decoder as shipped in udevbe/tinyh264 (Apache
# 2.0, see h264bsd/LICENSE) — the same code Espressif's esp_h264 wraps as a
# PREBUILT libtinyh264.a. Building it from source is what lets us fix what the
# prebuilt cannot: ~30 ms per frame regardless of content (two copies of every
# macroblock, full-frame deblocking), an "unchanged picture" signal for the
# display stage, -O2, and PIE SIMD kernels later. h264dec.c is our adapter.
file(GLOB H264BSD_SRCS "${CMAKE_CURRENT_LIST_DIR}/h264bsd/*.c")

idf_component_register(SRCS ${H264BSD_SRCS} "h264dec.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "h264bsd"
                       REQUIRES esp_timer esp_hw_support)

# The rest of the firmware is on -Og; the decoder is the one place where the
# optimiser is the point. Hantro's code was written for lint, not -Wall.
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    H264BSD_ESP_FASTPATH=1
    _ERROR_PRINT=1
    $<$<BOOL:${CONFIG_H264DEC_STATS}>:H264BSD_ESP_STATS=1>)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -O2
    -Wno-unused-but-set-variable
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-implicit-fallthrough
    -Wno-int-conversion
    -Wno-format
    -Wno-maybe-uninitialized
    -Wno-error=maybe-uninitialized
    -Wno-error=stringop-overflow
    -Wno-error=array-bounds)
