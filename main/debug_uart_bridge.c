#include "debug_uart_bridge.h"

#include "sdkconfig.h"

#if CONFIG_DEBUG_UART_BRIDGE

#include <stddef.h>

#include "esp_console.h"
#include "esp_log.h"
#include "linenoise/linenoise.h"

#include "debug_cmds.h"

static const char *TAG = "dbg_uart";

/* esp_console hands us argc/argv with argv[0] = the command name, which is
 * exactly what debug_cmds_run() expects — so every registered command shares
 * this one entry point and the implementations live in debug_cmds.c, next to
 * the Wi-Fi transport's copy. NULL sink = plain stdout, i.e. this console. */
static int console_dispatch(int argc, char **argv)
{
    return debug_cmds_run(argc, argv, NULL);
}

esp_err_t debug_uart_bridge_init(void)
{
    static bool inited;
    if (inited) return ESP_ERR_INVALID_STATE;

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "p4>";
    repl_cfg.task_stack_size = 8192;   /* room for base64 line + handlers */
    repl_cfg.max_cmdline_length = 256;

    /* Whichever console this board has. The Waveshare P4 talks over UART0
     * (a bridge chip on the USB port); the Guition JC4880 brings out no UART0
     * header and the S3 board's USB-C is the chip's own USB peripheral, so on
     * both of those the console is USB-Serial-JTAG — the only way a host
     * script can reach them — and scripts/uart_debug.py opens
     * /dev/cu.usbmodem* instead of a USB-serial adapter. */
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t dev_cfg =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&dev_cfg, &repl_cfg, &repl);
#else
    esp_console_dev_uart_config_t dev_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_uart(&dev_cfg, &repl_cfg, &repl);
#endif
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "repl init failed: %s", esp_err_to_name(err));
        return err;
    }

    debug_cmds_init();
    for (size_t i = 0; i < debug_cmds_count(); i++) {
        const esp_console_cmd_t cmd = {
            .command = debug_cmds_name(i),
            .help    = debug_cmds_help(i),
            .hint    = NULL,
            .func    = console_dispatch,
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    }

    /* Host is a script, not a human: force dumb mode so linenoise doesn't emit
     * cursor-probe escape sequences and command output stays deterministic. */
    linenoiseSetDumbMode(1);

    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "repl start failed: %s", esp_err_to_name(err));
        return err;
    }

    inited = true;
    ESP_LOGI(TAG, "serial debug bridge up (screenshot + touch injection)");
    return ESP_OK;
}

#else  /* !CONFIG_DEBUG_UART_BRIDGE */

esp_err_t debug_uart_bridge_init(void)
{
    return ESP_OK;
}

#endif /* CONFIG_DEBUG_UART_BRIDGE */
