/*
 * Enables VBAT routing for the ESP32-P4 LP power domain in active mode,
 * so the LP_TIMER and LP_STORE2/3 retention regs (where ESP-IDF stores
 * the RTC boot_time epoch) survive a full POR / EN-pin reset as long
 * as the CR2032 on H8 → ESP_VBAT is healthy. Net result: time(NULL)
 * keeps ticking across USB unplug, RESET button presses, and brownout.
 *
 * Background. ESP-IDF's built-in VBAT API only programs vddbat_mode=1
 * into PMU_MODE_LP_SLEEP (pmu_sleep.c:184), so the analog mux only
 * commits to VBAT as part of esp_deep_sleep_start(). On a cold POR
 * the chip never transitions through LP_SLEEP, so without intervention
 * the LP domain comes up freshly from VDDA, LP_TIMER zeroes, and the
 * boot_time epoch in LP_STORE2/3 is lost.
 *
 * What works: write the AUTO value (2) into BOTH LP_ACTIVE and LP_SLEEP
 * slots, set bod_source_sel=1 (so the LP brownout watches VBAT instead
 * of VDDA — otherwise BOD trips on VDDA collapse before AUTO can switch
 * over), and commit via PMU.vbat_cfg.sw_update. In AUTO the PMU picks
 * whichever rail is higher, so during normal operation LP runs from
 * VDDA and on USB unplug it falls through to the CR2032 on VBAT. The
 * 32.768 kHz Y2 crystal then keeps LP_TIMER running on battery.
 *
 * This is the configuration that the ESP-IoT-Solution docs hint at
 * (https://docs.espressif.com/projects/esp-iot-solution/en/latest/
 * low_power_solution/esp32p4_vbat.html) but ESP-IDF itself doesn't
 * apply on its own — "current scheme is for testing purposes only".
 * Verified working on hardware 2026-05-17 on a Waveshare ESP32-P4-
 * WIFI6-Touch-LCD-4.3 with a CR2032 in H8.
 *
 * Requires CONFIG_RTC_CLK_SRC_EXT_CRYS=y (otherwise the internal RC
 * slow clock isn't stable on battery), CONFIG_RTC_CLK_CAL_CYCLES=3000
 * (or EXT_CRYS calibration silently falls back to RC), and the CR2032
 * actually populated on H8. CONFIG_ESP_VBAT_INIT_AUTO=y is helpful for
 * battery-low diagnostics via esp_vbat_get_battery_state().
 */

#include "vbat_routing.h"

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_vbat.h"
#include "hal/pmu_types.h"
#include "soc/pmu_struct.h"

static const char *TAG = "vbat";

void vbat_routing_enable(void)
{
    PMU.lp_sys[PMU_MODE_LP_ACTIVE].dig_power.vddbat_mode    = 2;  /* AUTO */
    PMU.lp_sys[PMU_MODE_LP_ACTIVE].dig_power.bod_source_sel = 1;
    PMU.lp_sys[PMU_MODE_LP_SLEEP].dig_power.vddbat_mode     = 2;
    PMU.lp_sys[PMU_MODE_LP_SLEEP].dig_power.bod_source_sel  = 1;
    PMU.vbat_cfg.sw_update = 1;
    esp_rom_delay_us(500);

    esp_vbat_state_t bs = esp_vbat_get_battery_state();
    ESP_LOGI(TAG, "VBAT routing armed (ana=%u); coin cell: %s",
             (unsigned)PMU.vbat_cfg.ana_vddbat_mode,
             bs == ESP_VBAT_STATE_NORMAL     ? "NORMAL (>2.6V)" :
             bs == ESP_VBAT_STATE_LOWBATTERY ? "LOW — replace" :
                                               "CHARGING/?");
}
