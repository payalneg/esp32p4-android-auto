#pragma once

/* Boot-time "battery charged — reset trip?" prompt.
 *
 * battery_calc_voltage_boot_check() compares this power-on's pack voltage with
 * the previous one; a jump up means the pack was charged or swapped while the
 * unit was off. It used to reset the trip on its own — now it hands the
 * decision here and the rider answers on-screen (Reset / Keep). Modal on
 * lv_layer_top(), the same floor notif_toast uses, so it shows above any
 * dashboard theme. Auto-closes as "Keep" after CHARGE_PROMPT_TIMEOUT_MS so a
 * rider who ignores it (already moving) never loses the dashboard; the manual
 * reset icon is still there for later.
 *
 * Call charge_prompt_init() once the LVGL UI is up (after ui_mode_init) and
 * before the VESC CAN poller starts, so the callback is armed for the first
 * ESC reading. */
void charge_prompt_init(void);
