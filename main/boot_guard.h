#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* Start before risky initialisation; load the persistent record after NVS. */
void boot_guard_start(void);
void boot_guard_load(void);
void boot_guard_stage(const char *stage);
void boot_guard_ready(void);
void boot_guard_ui_tick(void);
/* Direct Android Auto video deliberately pauses LVGL; transitions only. */
void boot_guard_ui_required(bool required);
void boot_guard_ble_tick(void);
void boot_guard_nav_tick(void);
bool boot_guard_recovery(void);
bool boot_guard_safe_display(void);
/* A bounded exception for flash erase/program; never extended by a timer. */
void boot_guard_flash_begin(void);
void boot_guard_flash_end(void);
int boot_guard_status(char *buf, size_t capacity);
/* Version 1, opcode, random per-boot nonce LE32. Commands execute off BLE. */
esp_err_t boot_guard_command(const uint8_t *data, size_t size);
