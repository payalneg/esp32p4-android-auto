#include "aa_overclock.h"

#include "sdkconfig.h"

#if CONFIG_AA_OVERCLOCK_400

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "hal/clk_tree_ll.h"
#include "hal/regi2c_ctrl_ll.h"
#include "esp_private/regi2c_ctrl.h"
#include "soc/clk_tree_defs.h"

static const char *TAG = "overclock";

/* Replicates the static rtc_clk_cpll_configure() from
 * esp-idf/components/esp_hw_support/port/esp32p4/rtc_clk.c. We can't call
 * the public rtc_clk_cpu_freq_set_config() because under
 * CONFIG_ESP32P4_SELECTS_REV_LESS_V3 (rev 1.x silicon, our case) the helper
 * rtc_clk_cpu_freq_to_cpll_mhz() abort()s on freq != {360,180,90}. So we
 * drive the LL primitives directly. */
static IRAM_ATTR void cpll_recalibrate_to_400(void)
{
    clk_ll_cpll_set_freq_mhz(400);
    ANALOG_CLOCK_ENABLE();
    regi2c_ctrl_ll_cpll_calibration_start();
    clk_ll_cpll_set_config(400, 40);
    while (!regi2c_ctrl_ll_cpll_calibration_is_done()) { }
    esp_rom_delay_us(10);
    regi2c_ctrl_ll_cpll_calibration_stop();
    ANALOG_CLOCK_DISABLE();
}

IRAM_ATTR esp_err_t aa_overclock_400mhz_apply(void)
{
    uint32_t prev = esp_cpu_intr_get_enabled_mask();
    esp_cpu_intr_disable(prev);

    /* Park CPU on XTAL@40MHz before touching CPLL. APB temporarily collapses
     * to 40MHz; that's why we run very early — no peripherals are clocked
     * across this window, and console UART output during it would be garbled
     * but we don't print here. */
    clk_ll_cpu_set_src(SOC_CPU_CLK_SRC_XTAL);
    clk_ll_cpu_set_divider(1, 0, 0);
    clk_ll_mem_set_divider(1);
    clk_ll_sys_set_divider(1);
    clk_ll_apb_set_divider(1);
    clk_ll_bus_update();
    esp_rom_set_cpu_ticks_per_us(40);

    cpll_recalibrate_to_400();

    /* Engage CPLL@400: cpu_div=1, mem_div=2, sys_div=1, apb_div=2.
     * → CPU=400, MEM=200, SYS=200, APB=100. Order: APB→SYS→MEM→CPU
     * (upscale; avoid illegal intermediate states per rtc_clk.c comment). */
    clk_ll_apb_set_divider(2);
    clk_ll_bus_update();
    clk_ll_sys_set_divider(1);
    clk_ll_bus_update();
    clk_ll_mem_set_divider(2);
    clk_ll_bus_update();
    clk_ll_cpu_set_divider(1, 0, 0);
    clk_ll_bus_update();
    clk_ll_cpu_set_src(SOC_CPU_CLK_SRC_CPLL);
    esp_rom_set_cpu_ticks_per_us(400);

    esp_cpu_intr_enable(prev);

    ESP_LOGW(TAG, "CPU @ 400 MHz (UNSUPPORTED — rev <3.0 silicon)");
    return ESP_OK;
}

#else  /* !CONFIG_AA_OVERCLOCK_400 */

esp_err_t aa_overclock_400mhz_apply(void)
{
    return ESP_OK;
}

#endif
