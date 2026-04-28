# Camera Integration Architecture

## Overview

The JC4880P433C BSP provides camera hardware support (I2C bus, GPIO configuration) but **does not include camera sensor drivers**. Camera sensor drivers are maintained separately in the [esp-video-components](https://github.com/espressif/esp-video-components) repository.

## Architecture

```
┌─────────────────────────────────────┐
│   Your Application/Phone Project   │
│  (e.g., phone_p4_JC4880P433C)      │
└──────────────┬──────────────────────┘
               │
               ├──> esp32_p4_jc4880p433c_bsp (this BSP)
               │    └─> Provides: I2C bus init, GPIO config
               │    └─> Does NOT provide: Camera sensor drivers
               │
               └──> esp-video-components
                    ├─> esp_cam_sensor (OV02C10, SC2336, etc.)
                    ├─> esp_video (video device framework)
                    ├─> esp_sccb_intf (I2C/SCCB interface)
                    └─> esp_ipa (image processing)
```

## BSP Responsibilities

The BSP (`esp32_p4_jc4880p433c_bsp`) provides:

1. **Hardware Configuration**
   - I2C pin definitions (GPIO7/8 shared with touch)
   - I2C bus initialization via `bsp_i2c_init()`
   - Camera reset/power-down GPIO management
   - Board-specific pin mappings

2. **Camera Hardware API** (`include/bsp/camera.h`)
   ```c
   esp_err_t bsp_camera_init(const bsp_camera_config_t *config, 
                              i2c_master_bus_handle_t *i2c_handle);
   bsp_camera_config_t bsp_camera_get_default_config(void);
   ```

## Camera Sensor Integration

Camera sensor drivers (OV02C10, SC2336, etc.) come from `esp-video-components`:

### Option 1: Use Official Component Registry (Recommended for Production)
```yaml
# In your project's idf_component.yml
dependencies:
  espressif/esp_cam_sensor: "^1.5.0"  # Includes OV02C10
  espressif/esp_video: "*"
  espressif/esp_sccb_intf: "*"
  espressif/esp_ipa: "*"
```

### Option 2: Use Local Development Copy (For Development)
```yaml
# In your project's idf_component.yml
dependencies:
  esp_cam_sensor:
    version: "*"
    override_path: "../esp-video-components/esp_cam_sensor"
  esp_video:
    version: "*"
    override_path: "../esp-video-components/esp_video"
  # ... other components
```

### Option 3: Use Your Fork (For Custom Sensors)
```yaml
# In your project's idf_component.yml
dependencies:
  csvke/esp_cam_sensor:
    git: "https://github.com/csvke/esp-video-components.git"
    path: "esp_cam_sensor"
    version: "feat/add-ov02c10-sensor"  # or "main" after PR merge
```

## Supported Camera Sensors

### OV02C10 (OmniVision 2MP MIPI CSI Sensor)

**Current Status (October 13, 2025):**
- ✅ Driver fully implemented and tested
- 🔄 **PR pending approval**: [espressif/esp-video-components#46](https://github.com/espressif/esp-video-components/pull/46)
- 📦 **Until official merge**: Use fork at `https://github.com/csvke/esp-video-components`

**Specifications:**
- **Model**: OV02C10 (OmniVision)
- **Resolution**: Up to 1920x1080 (2MP)
- **Output**: 10-bit RAW Bayer GBRG
- **Interface**: MIPI CSI (1-lane or 2-lane)
- **I2C Address**: 0x36 (7-bit)
- **Modes**:
  - 1288x728 @ 30fps (1-lane)
  - 1920x1080 @ 30fps (1-lane)
  - 1920x1080 @ 30fps (2-lane)

## Usage Example

```c
#include "bsp/esp-bsp.h"
#include "bsp/camera.h"
#include "esp_video_init.h"

void app_camera_init(void) {
    // 1. Initialize BSP camera hardware (I2C, GPIO)
    i2c_master_bus_handle_t i2c_handle;
    ESP_ERROR_CHECK(bsp_camera_init(NULL, &i2c_handle));
    
    // 2. Initialize video system with camera sensor
    esp_video_init_csi_config_t csi_config = {
        .sccb_config = {
            .init_sccb = false,  // Use BSP's I2C
            .i2c_handle = i2c_handle,
            .freq = 400000,
        },
    };
    
    esp_video_init_config_t video_config = {
        .csi = &csi_config,
    };
    
    ESP_ERROR_CHECK(esp_video_init(&video_config));
    
    // 3. Open video device and start streaming
    int fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDWR);
    // ... configure and use video device
}
```

## Hardware Details

### I2C Bus Sharing
The camera I2C bus (GPIO7/8) is **shared** with the GT911 touch controller:
- **SCL**: GPIO8 (CSI pin 13)
- **SDA**: GPIO7 (CSI pin 14)
- **Frequency**: 400kHz
- **Bus Handle**: Obtained via `bsp_i2c_get_handle()`

### Pin Configuration
```
Camera Module Connector:
├─ Pin 13: I2C_SCL (GPIO8) - Shared with GT911
├─ Pin 14: I2C_SDA (GPIO7) - Shared with GT911
├─ MIPI CSI D0+/D0- (lane 0)
├─ MIPI CSI D1+/D1- (lane 1, optional)
└─ MIPI CSI CLK+/CLK-
```

## References

- [ESP-IDF Camera Driver Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32p4/api-reference/peripherals/camera_driver.html)
- [esp-video-components Repository](https://github.com/espressif/esp-video-components)
- [OV02C10 Driver PR](https://github.com/espressif/esp-video-components/pulls) (pending)

## Contributing

To add support for new camera sensors:
1. Fork [esp-video-components](https://github.com/espressif/esp-video-components)
2. Follow the [Camera Sensor Driver Guide](https://github.com/espressif/esp-video-components/blob/master/esp_cam_sensor/README.md)
3. Submit a PR to the official repository

Camera sensor drivers should **not** be added to this BSP repository.
