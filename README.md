# M5Cube - Orientation-Based Multiplayer Timer

A universal multiplayer game timer application for both M5Stack Fire and CoreS3, inspired by the DGT Cube. The device automatically detects orientation using the built-in IMU sensor to manage up to 5 individual player timers. Device type is automatically detected and the UI adapts accordingly.

## Features

**Universal Design** - Single codebase supports both M5Stack Fire and CoreS3 with automatic device detection and optimized UI for each platform.

**Orientation-Based Player Switching** - Simply rotate the device to switch between 5 different player timers using IMU sensor detection.

**Overtime Tracking** - When a timer reaches zero, it continues counting upward in red to track overtime duration.

**Power Management** - Automatic screen shutoff when placed face-down, reduced CPU frequency, and optimized display updates for extended battery life.

**Adaptive UI** - Running state disables reset functionality with visual feedback (blue text for disabled buttons).

## Supported Devices

| Device | Input Method | Features |
|--------|--------------|----------|
| **M5Stack Fire** | Physical buttons (A/B/C) | Traditional 3-button operation |
| **M5Stack CoreS3** | Touch screen + Physical buttons | Touch button UI display |

## DGT Cube Comparison

M5Cube reimplements the DGT Cube board game timer concept for the M5Stack platform:

| Feature | DGT Cube | M5Cube |
|---------|----------|---------|
| Players | 6 | 5 |
| Display | Dedicated face displays | Single rotating display |
| Pause Method | Special base | Face-down detection |
| Platform | Proprietary hardware | M5Stack |

## Setup Guide

### Arduino IDE Environment

Follow the detailed setup guides from M5Stack official documentation:

- **M5Stack Fire**: https://docs.m5stack.com/ja/arduino/m5fire/program
- **M5Stack CoreS3**: https://docs.m5stack.com/ja/arduino/m5cores3/program

### Quick Setup Steps

1. **Install Arduino IDE** (version 1.8.19 or later)

2. **Add M5Stack Board Definitions**
   - Arduino IDE > Preferences > Additional Board Manager URLs
   - Add: `https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json`

3. **Install M5Stack Boards**
   - Tools > Board > Boards Manager
   - Search "M5Stack" and install

4. **Install M5Unified Library**
   - Tools > Manage Libraries
   - Search "M5Unified" and install

5. **Board Configuration**
   - **Fire**: Tools > Board > M5Stack-Fire
   - **CoreS3**: Tools > Board > M5Stack-CoreS3

### Upload Program

1. Open `M5Cube_Universal.ino` in Arduino IDE
2. Connect device via USB-C cable
3. Select appropriate port
4. Click upload button

## Usage

### Initial Setup
1. Power on device to enter setup mode
2. Configure timer duration (MM:SS format)
3. Press Start button to begin timing

### Controls

#### M5Stack Fire (Physical Buttons)
| Mode | A Button | B Button | C Button |
|------|----------|----------|----------|
| **Setup** | +1 (increment) | Start | Next (digit) |
| **Stopped** | Reset | Start | - |
| **Running** | (Reset) | Stop | - |

#### M5Stack CoreS3 (Touch Screen)
| Mode | Left Touch | Center Touch | Right Touch |
|------|------------|--------------|-------------|
| **Setup** | +1 | START | NEXT |
| **Stopped** | RESET | START | - |
| **Running** | (RESET) | STOP | - |

*Note: Parentheses indicate disabled state (blue text)*

### Player Switching

The device automatically switches players based on orientation:

| Orientation | Player | Acceleration Axis |
|-------------|--------|-------------------|
| Front edge up | Player 1 | Y-axis positive |
| Right edge up | Player 2 | X-axis positive |
| Back edge up | Player 3 | Y-axis negative |
| Left edge up | Player 4 | X-axis negative |
| Flat (screen up) | Player 5 | Z-axis positive |

## Technical Specifications

### Hardware Requirements
- M5Stack Fire or M5Stack CoreS3
- 6-axis IMU sensor (orientation detection)
- Built-in speaker (audio feedback)
- Battery power recommended

### Power Optimization
- CPU frequency reduced to 80MHz (from 240MHz)
- Screen brightness set to 50%
- Face-down detection for automatic screen shutoff
- Partial screen updates for performance

### Device Detection Logic
```cpp
// Detection priority
1. M5.getBoard() for accurate identification
2. Touch capability fallback detection
3. Default Fire-compatible operation
```

## Customization

### Adjustable Constants
```cpp
const float ACCEL_THRESHOLD = 0.9f;        // Orientation sensitivity
const int TONE_FREQ_HIGH = 2800;           // Switch sound frequency
const int BUTTON_TEXT_SIZE = 2.5;          // Touch button size (CoreS3)
```

### Color Settings
```cpp
const uint32_t COLOR_WHITE = 0xFFFFFF;     // Normal display
const uint32_t COLOR_RED = 0xFF0000;       // Overtime mode
const uint32_t COLOR_BLUE = 0x000066;      // Disabled buttons
```

## Troubleshooting

**Orientation detection unstable** - Adjust `ACCEL_THRESHOLD` between 0.5-1.0 or place device on stable surface.

**Compile errors** - Verify M5Unified library is latest version and board settings are correct.

**Screen unresponsive** - Press any button to wake screen from power-save mode or press reset button.

**Poor battery life** - Lower screen brightness further or place face-down when not in use.

## Future Development

**Potential enhancements** customizable audio settings, game history logging, and WiFi remote control capabilities.

**Current limitations** include 59:59 maximum timer duration, possible false triggers with rapid movement, and unverified compatibility with M5Stack models other than Fire and CoreS3.

## License

This project is open source and freely available for personal and commercial use.
