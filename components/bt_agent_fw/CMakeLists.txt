# BT agent firmware blob — embedded gzipped into the host bin and
# decompressed into PSRAM on first access by bt_agent_fw_data(). Saves
# ~427 KiB of host flash vs embedding the raw .bin (firmware compiles
# down to ~57% with gzip -9). PSRAM has 32 MiB so the temp uncompressed
# copy is free.
#
# Re-pack via tools/pack_fw_blobs.sh after rebuilding the BT agent.

set(_blob_gz "bt_agent_fw.bin.gz")
set(_embed_files "")

if(CONFIG_BT_AGENT_OTA_ENABLED)
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_blob_gz}")
        message(FATAL_ERROR
            "BT_AGENT_OTA_ENABLED=y but ${CMAKE_CURRENT_LIST_DIR}/${_blob_gz} "
            "is missing. Build the BT agent and run tools/pack_fw_blobs.sh.")
    endif()
    set(_embed_files "${_blob_gz}")
endif()

# Single registration — IDF's component scanner only honours the first
# idf_component_register() it sees, and an if/else with two of them leaves
# priv_reqs empty for whichever branch wins. Always register the same way;
# EMBED_FILES is empty when OTA is off and the .c file falls back to the
# stub branch via #ifdef.
idf_component_register(
    SRCS "bt_agent_fw.c"
    INCLUDE_DIRS "."
    EMBED_FILES "${_embed_files}"
    PRIV_REQUIRES espressif__zlib esp_psram log
)
