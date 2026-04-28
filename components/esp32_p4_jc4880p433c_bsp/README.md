# ESP32-P4 JC4880P433C Board Support Package

Production-ready BSP for the JC4880P433C development board with ESP32-P4, featuring MIPI-DSI display and capacitive touch.

## 🔧 Hardware Specifications

| Component | Details |
|-----------|---------|
| **MCU** | ESP32-P4 RISC-V Dual-Core @ 360MHz |
| **Display** | 4.3" 480x800 ST7701S MIPI-DSI LCD |
| **Touch** | GT911 I2C Capacitive Touch Controller |
| **Camera** | OV02C10 2MP Camera Sensor (I2C control) |
| **Backlight** | PWM-controlled (configurable GPIO) |
| **Interface** | MIPI-DSI 2-lane, RGB565 color |
| **I2C** | SCL=GPIO8, SDA=GPIO7 (shared bus, external pull-ups) |
| **Power** | LDO ch3: 2500mV for MIPI PHY |

## ✨ Features

- ✅ **ST7701S MIPI-DSI Display**: Full initialization with timing configuration
- 👆 **GT911 Touch Controller**: I2C communication with auto-detection support
- � **OV02C10 Camera Sensor**: 2MP camera with I2C control on shared bus
- �💡 **PWM Backlight Control**: Smooth brightness adjustment (0-100%)
- 🎨 **LVGL Integration**: Direct esp_lvgl_port support with DSI configuration
- 💾 **SPIFFS Support**: Auto-format on first boot for seamless user experience
- 🔌 **Shared I2C Bus**: Centralized I2C management for touch and camera peripherals
- 📦 **Modular Architecture**: Swappable components (display/touch/camera) vs integrated (SPIFFS/audio/RS485)

## 📦 Installation

### Option 1: Local Component (Recommended for Development)

Add to your project's `CMakeLists.txt`:
```cmake
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_CURRENT_LIST_DIR}/../esp32_p4_jc4880p433c_bsp"
)
```

Then clone this repository alongside your project:
```bash
cd your_workspace
git clone https://github.com/csvke/esp32_p4_jc4880p433c_bsp.git
```

### Option 2: ESP Component Manager

Add to your `idf_component.yml`:
```yaml
dependencies:
  csvke/esp32_p4_jc4880p433c_bsp:
    version: ">=1.0.0"
    git: https://github.com/csvke/esp32_p4_jc4880p433c_bsp.git
```

## 🏗️ Architecture

The BSP follows a **modular architecture** that separates swappable components from integrated components:

### Modular Components (Connected via FPC/Connectors)
These components can be swapped by developers for alternative hardware:

| Module | File | Description |
|--------|------|-------------|
| **Display** | `bsp_display.c` | ST7701S LCD with MIPI-DSI, backlight control, LVGL integration |
| **Touch** | `bsp_touch.c` | GT911 capacitive touch controller via I2C |
| **Camera** | `bsp_camera.c` | OV02C10 camera sensor with I2C control |
| **I2C Bus** | `bsp_i2c.c` | Shared I2C bus manager for touch and camera |

### Integrated Components (Soldered to Board)
These components are part of the JC4880P433C board design:

| Module | File | Description |
|--------|------|-------------|
| **Board BSP** | `jc4880p433c_bsp.c` | SPIFFS storage, SD card slot, audio codec, RS485 interface |

This architecture allows developers to:
- 🔄 Swap display panels without touching board-level code
- 🔄 Replace touch controllers with different models
- 🔄 Use alternative camera sensors
- ✅ Keep board-specific code stable and isolated

## 🚀 Usage

### Basic Display Initialization

```c
#include "bsp/esp-bsp.h"

void app_main(void)
{
    // Start display with default configuration
    lv_display_t *disp = bsp_display_start();
    
    // Turn on backlight
    bsp_display_backlight_on();
    
    // Your LVGL code here
}
```

### Custom Display Configuration

```c
#include "bsp/display.h"

void app_main(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = 480 * 80,  // 480x80 buffer
        .double_buffer = false,
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = true,     // Enable software rotation
        }
    };
    
    lv_display_t *disp = bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();
}
```

### Touch Controller

```c
#include "bsp/touch.h"

void app_main(void)
{
    // Initialize touch (automatically done by bsp_display_start)
    esp_lcd_touch_handle_t tp;
    bsp_touch_config_t touch_cfg = { .dummy = NULL };
    bsp_touch_new(&touch_cfg, &tp);
    
    // Touch is registered with LVGL automatically
}
```

### Camera Sensor

```c
#include "bsp/camera.h"

void app_main(void)
{
    // Initialize camera hardware
    bsp_camera_config_t cam_cfg = {
        .reset_pin = GPIO_NUM_NC,  // Optional reset pin
        .pwdn_pin = GPIO_NUM_NC,   // Optional power-down pin
    };
    
    esp_err_t ret = bsp_camera_init(&cam_cfg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Camera hardware initialized");
        // Use with V4L2 or other camera frameworks
    }
}
```

### SPIFFS Storage

```c
#include "bsp/esp-bsp.h"

void app_main(void)
{
    // Mount SPIFFS (auto-formats on first boot)
    esp_err_t ret = bsp_spiffs_mount();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted successfully");
        // Read/write files in /spiffs/
    }
}
```

## 📖 API Reference

### Display Functions

```c
// Initialize with default config
lv_display_t *bsp_display_start(void);

// Initialize with custom config
lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg);

// Backlight control
esp_err_t bsp_display_backlight_on(void);
esp_err_t bsp_display_brightness_set(uint8_t brightness_percent);

// LVGL thread safety
bool bsp_display_lock(uint32_t timeout_ms);
void bsp_display_unlock(void);
```

### Touch Functions

```c
// Initialize touch controller
esp_err_t bsp_touch_new(const bsp_touch_config_t *config, 
                        esp_lcd_touch_handle_t *ret_touch);
```

### Camera Functions

```c
// Initialize camera hardware (I2C control)
esp_err_t bsp_camera_init(const bsp_camera_config_t *config);

// Deinitialize camera hardware
void bsp_camera_deinit(void);
```

### I2C Functions

```c
// Initialize shared I2C bus (called internally by touch/camera)
esp_err_t bsp_i2c_init(void);

// Get I2C bus handle for custom devices
i2c_master_bus_handle_t bsp_i2c_get_handle(void);
```

### Storage Functions

```c
// Mount SPIFFS partition (auto-formats if needed)
esp_err_t bsp_spiffs_mount(void);

// Mount SD card (if available)
esp_err_t bsp_sdcard_mount(void);
```

## ⚙️ Configuration

### Kconfig Options

Access via `idf.py menuconfig` → `Component config` → `Board Support Package (JC4880P433C)`:

| Option | Description | Default |
|--------|-------------|---------|
| `BSP_JC4880P443C_LCD_BL_GPIO` | Backlight GPIO pin | GPIO_NUM_NC |
| `BSP_JC4880P443C_LCD_RST_GPIO` | LCD reset GPIO | GPIO_NUM_NC |
| `BSP_TOUCH_ENABLED` | Enable touch support | Yes |

### Display Timing (ST7701S MIPI-DSI)

```c
// Pixel clock: 27MHz (480x800 @ ~60Hz)
// MIPI-DSI: 2-lane, 500Mbps/lane
// Color format: RGB565
// Rotation: Portrait (480x800)
```

### Pin Assignments

| Function | GPIO | Description |
|----------|------|-------------|
| I2C SCL | GPIO 8 | Touch controller clock |
| I2C SDA | GPIO 7 | Touch controller data |
| Backlight | Configurable | PWM backlight control |
| Reset | Configurable | LCD reset (optional) |

## 🏗️ Technical Details

### BSP File Structure
```
esp32_p4_jc4880p433c_bsp/
├── src/
│   ├── bsp_display.c       # Display hardware (ST7701S MIPI-DSI, backlight)
│   ├── bsp_touch.c         # Touch controller (GT911 via I2C)
│   ├── bsp_camera.c        # Camera sensor (OV02C10 via I2C)
│   ├── bsp_i2c.c           # Shared I2C bus manager
│   └── jc4880p433c_bsp.c   # Board integration (SPIFFS, SD, audio, RS485)
├── include/bsp/
│   ├── display.h           # Display API
│   ├── touch.h             # Touch API
│   ├── camera.h            # Camera API
│   └── esp-bsp.h           # Main BSP header
└── CMakeLists.txt
```

### Display Initialization Sequence
1. Enable MIPI-DSI PHY power (LDO ch3: 2500mV)
2. Create MIPI-DSI bus (2-lane, 500Mbps)
3. Initialize ST7701S panel with timing configuration
4. Register with LVGL via esp_lvgl_port
5. Configure touch input device

### Camera Initialization Sequence
1. Initialize shared I2C bus (if not already initialized)
2. Configure camera reset and power-down pins (optional)
3. Initialize OV02C10 sensor via I2C
4. Camera ready for V4L2 or other video frameworks

### SPIFFS Configuration
- **Partition Label**: `storage`
- **Size**: 4MB (configurable in partitions.csv)
- **Auto-Format**: Enabled (formats on first mount failure)
- **Mount Point**: `/spiffs`

### I2C Configuration (Shared Bus)
- **Port**: I2C_NUM_0
- **GPIOs**: SDA=GPIO7, SCL=GPIO8
- **Clock Speed**: 400kHz
- **Pull-ups**: External hardware pull-ups (internal disabled)
- **Glitch Filter**: 7 clock cycles
- **Shared Devices**: GT911 touch (0x14/0x5D), OV02C10 camera (0x36)

## 🐛 Troubleshooting

### Display Issues
- **No display output**: Check MIPI-DSI cable connections and LDO power
- **Wrong colors**: Verify RGB565 color format and byte order
- **Flickering**: Adjust pixel clock or MIPI-DSI data rate

### Touch Issues
- **Touch not responding**: 
  - Verify I2C connections (SCL=GPIO8, SDA=GPIO7)
  - Check GT911 power supply
  - Use `i2cdetect` to scan bus (GT911 address: 0x14 or 0x5D)
- **I2C warnings**: Informational only - external pull-ups are present

### Camera Issues
- **Camera not detected**:
  - Verify I2C connections (shared with touch controller)
  - Check camera power supply and reset/pwdn pins
  - Use `i2cdetect` to scan bus (OV02C10 address: 0x36)
- **Shared I2C conflicts**: Camera and touch share I2C bus - both should work simultaneously

### SPIFFS Issues
- **Mount failed**: First boot auto-formats (takes ~1 second)
- **No space**: Check partition size in partitions.csv
- **Read errors**: Reflash SPIFFS partition or erase flash

## 📋 Dependencies

- **ESP-IDF**: v5.5.1 or later
- **esp_lcd_st7701**: v1.1.4 (MIPI-DSI LCD driver)
- **esp_lcd_touch_gt911**: v1.1.3 (Touch controller driver)
- **esp_cam_sensor**: Camera sensor drivers (OV02C10 support)
- **esp_lvgl_port**: v2.6.2 (LVGL integration)
- **esp_driver_i2c**: ESP-IDF I2C master driver
- **esp_driver_ledc**: ESP-IDF LEDC PWM driver (backlight control)

## 📚 References

- [ST7701S Datasheet](https://www.sitronix.com.tw/en/product/Driver/mobile_display.html)
- [GT911 Datasheet](https://github.com/goodix/gt9xx_driver)
- [ESP32-P4 MIPI-DSI](https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/api-reference/peripherals/lcd/mipi_dsi.html)
- [ESP-IDF I2C Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/api-reference/peripherals/i2c.html)

## 📝 License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

## 🔗 Related Projects

- [phone_p4_JC4880P433C](https://github.com/csvke/phone_p4_JC4880P433C) - Brookesia phone UI demo project
- [ESP-Brookesia](https://github.com/espressif/esp-brookesia) - Phone UI framework

## 🤝 Contributing

Contributions are welcome! Please submit issues or pull requests on GitHub.

## 👤 Author

**csvke**
- GitHub: [@csvke](https://github.com/csvke)

---

**Status**: ✅ Production Ready | **Last Updated**: October 13, 2025