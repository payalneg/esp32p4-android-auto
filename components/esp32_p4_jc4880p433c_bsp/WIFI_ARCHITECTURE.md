# WiFi Architecture on JC4880P443C Board

## Hardware Overview

The JC4880P443C board features a **dual-chip architecture** for WiFi connectivity:

- **ESP32-P4** (Primary MCU)
  - RISC-V dual-core @ 360MHz
  - 480x800 MIPI-DSI display controller
  - **No integrated WiFi/Bluetooth**
  - Acts as **Host** in ESP-Hosted architecture

- **ESP32-C6** (WiFi Co-processor)
  - RISC-V single-core
  - WiFi 6 (802.11ax) + Bluetooth 5 (LE)
  - Acts as **Slave** in ESP-Hosted architecture

## Communication Interface

**Interface**: SDIO (Secure Digital Input/Output)
**Configuration**: 4-bit bus @ 40MHz
**Protocol**: ESP-Hosted

### SDIO Pin Mapping (ESP32-P4 Side)

| Function | GPIO | Description |
|----------|------|-------------|
| CMD | GPIO19 | Command line |
| CLK | GPIO18 | Clock signal |
| D0 | GPIO14 | Data line 0 |
| D1 | GPIO15 | Data line 1 |
| D2 | GPIO16 | Data line 2 |
| D3 | GPIO17 | Data line 3 |
| RESET | GPIO54 | C6_CHIP_PU (Reset control for ESP32-C6) |

## ESP-Hosted Architecture

```
┌─────────────────┐         SDIO         ┌─────────────────┐
│   ESP32-P4      │◄──────────────────────►│   ESP32-C6      │
│   (Host)        │   CMD/CLK/D0-D3       │   (Slave)       │
│                 │   GPIO 14-19          │                 │
│  - Display      │                       │  - WiFi 6       │
│  - Apps         │   GPIO 54 (Reset)     │  - Bluetooth 5  │
│  - LVGL UI      │────────────────────────►│                 │
└─────────────────┘                       └─────────────────┘
        │                                         │
        │                                         │
        ▼                                         ▼
  WiFi APIs                                  WiFi Radio
  (esp_wifi_*)                              (802.11ax)
```

### How It Works

1. **ESP-Hosted Driver** on ESP32-P4 communicates with ESP32-C6 via SDIO
2. **WiFi Remote Library** provides standard ESP-IDF WiFi APIs (esp_wifi_*)
3. Applications use **standard WiFi APIs** - no custom code needed
4. ESP32-C6 runs firmware that handles actual WiFi/BLE operations
5. Data flows transparently between P4 apps and C6 radio

## Required Configuration

### sdkconfig Settings

```ini
# Enable ESP-Hosted
CONFIG_ESP_HOSTED_ENABLED=y
CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE=y
CONFIG_ESP_HOSTED_IDF_SLAVE_TARGET="esp32c6"

# SDIO Configuration
CONFIG_ESP_HOSTED_SDIO_SLOT_1=y
CONFIG_ESP_HOSTED_SDIO_4_BIT_BUS=y
CONFIG_ESP_HOSTED_SDIO_CLOCK_FREQ_KHZ=40000

# SDIO Pin Configuration (ESP32-P4)
CONFIG_ESP_HOSTED_SDIO_PIN_CMD=19
CONFIG_ESP_HOSTED_SDIO_PIN_CLK=18
CONFIG_ESP_HOSTED_SDIO_PIN_D0=14
CONFIG_ESP_HOSTED_SDIO_PIN_D1=15
CONFIG_ESP_HOSTED_SDIO_PIN_D2=16
CONFIG_ESP_HOSTED_SDIO_PIN_D3=17
CONFIG_ESP_HOSTED_SDIO_GPIO_RESET_SLAVE=54

# WiFi Remote (provides standard WiFi APIs)
CONFIG_ESP_WIFI_REMOTE_ENABLED=y
CONFIG_ESP_WIFI_REMOTE_LIBRARY_HOSTED=y
CONFIG_ESP_WIFI_REMOTE_EAP_ENABLED=y

# WiFi Buffer Configuration
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=16
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_TX_BUFFER_TYPE=0
CONFIG_ESP_WIFI_STATIC_TX_BUFFER_NUM=16
CONFIG_ESP_WIFI_CACHE_TX_BUFFER_NUM=32

# ESP-Hosted Optimization
CONFIG_ESP_HOSTED_SDIO_OPTIMIZATION_RX_STREAMING_MODE=y
CONFIG_ESP_HOSTED_SDIO_TX_Q_SIZE=20
CONFIG_ESP_HOSTED_SDIO_RX_Q_SIZE=20
CONFIG_ESP_HOSTED_USE_MEMPOOL=y

# PSRAM WiFi Support
CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y
```

### Dependencies (idf_component.yml)

```yaml
dependencies:
  espressif/esp_hosted:
    version: "^1.0.0"
  espressif/esp_wifi_remote:
    version: "^0.3.0"
```

## Usage in Application Code

### Standard WiFi APIs Work Transparently

```c
#include "esp_wifi.h"
#include "esp_event.h"

// Initialize WiFi (standard ESP-IDF way)
esp_netif_init();
esp_event_loop_create_default();
esp_netif_create_default_wifi_sta();

wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
esp_wifi_init(&cfg);
esp_wifi_set_mode(WIFI_MODE_STA);

// Scan for WiFi networks
wifi_scan_config_t scan_config = {
    .ssid = NULL,
    .bssid = NULL,
    .channel = 0,
    .show_hidden = false
};
esp_wifi_scan_start(&scan_config, false);

// Get scan results
uint16_t ap_count = 0;
esp_wifi_scan_get_ap_num(&ap_count);
wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
esp_wifi_scan_get_ap_records(&ap_count, ap_list);

// Connect to WiFi
wifi_config_t wifi_config = {
    .sta = {
        .ssid = "YourSSID",
        .password = "YourPassword",
    },
};
esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
esp_wifi_start();
esp_wifi_connect();
```

**No custom code needed!** ESP-Hosted + WiFi Remote makes it transparent.

## ESP32-C6 Firmware

The ESP32-C6 needs to run **ESP-Hosted slave firmware**. This is typically:
- Pre-flashed by manufacturer, OR
- Needs to be flashed separately using ESP-Hosted example

### Flashing C6 Firmware (if needed)

1. Get ESP-Hosted repository:
```bash
git clone https://github.com/espressif/esp-hosted.git
cd esp-hosted/esp_hosted_ng/esp/esp_driver/network_adapter
```

2. Build for ESP32-C6:
```bash
idf.py set-target esp32c6
idf.py menuconfig  # Configure SDIO interface
idf.py build
idf.py flash
```

## RMII Pins (Not Used on This Board)

The following RMII pins mentioned in schematic are **NOT used** for WiFi:
- RMII_TXD1 = GPIO35
- RMII_TXD0 = GPIO34
- MDC = GPIO31
- RMII_RXD1 = GPIO30
- RMII_RXD0 = GPIO29
- RMII_RXDV = GPIO28
- MDIO = GPIO52
- PHY_RSTN = GPIO51
- RMII_CLK = GPIO50
- RMII_TXEN = GPIO49

These pins are likely for:
- Future Ethernet PHY support
- Alternative board configurations
- Different product variants

**For WiFi, only SDIO pins (14-19, 54) are used.**

## References

- **Manufacturer Example**: `/Users/frankieyuen/JC4880P443C/Manufacturer Docs/JC4880P443C_I_W/1-Demo/idf_examples/ESP-IDF/xiaozhi-esp32`
- **ESP-Hosted Documentation**: https://github.com/espressif/esp-hosted
- **ESP WiFi Remote**: https://components.espressif.com/components/espressif/esp_wifi_remote
- **ESP32-P4 Datasheet**: https://www.espressif.com/sites/default/files/documentation/esp32-p4_datasheet_en.pdf
- **ESP32-C6 Datasheet**: https://www.espressif.com/sites/default/files/documentation/esp32-c6_datasheet_en.pdf

## Next Steps

1. Add ESP-Hosted and WiFi Remote dependencies to `idf_component.yml`
2. Add configuration to `sdkconfig.defaults`
3. Initialize WiFi in `main.cpp`
4. Add WiFi scanning/connection code to Settings app
5. Test on device

---

**Note**: This is a hardware-accelerated WiFi solution that provides full ESP-IDF WiFi API compatibility while using ESP32-P4's superior display capabilities combined with ESP32-C6's WiFi 6 radio.
