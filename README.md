了解しました。以下に、先ほどの英訳READMEを**そのままMarkdown形式**で整形したものを記載します。GitHubなどでの使用にそのまま適しています。

---

```markdown
# M5Cube - Advanced Orientation-Based Multiplayer Timer

A comprehensive multiplayer game timer app compatible with M5Stack Fire and CoreS3. It features dual timer modes, customizable sound alerts, and intelligent orientation-based player switching. The app automatically adapts its interface based on the device's hardware, offering professional-grade timing functions for board games.

## Key Features

### Dual Timer Modes  
- **Game Timer**: Classic chess-clock style with separate time pools for each player  
- **Turn Timer**: Fixed time per turn that resets upon player switch

### Smart Setup System  
- Initial sound configuration  
- Guided timer mode selection  
- Digit-based intuitive time editing

### Advanced Sound System  
- Toggleable sound alerts  
- Multi-phase countdown warnings (10s, 5s, 4s–1s)  
- Distinct tones for different events

### Intelligent Navigation  
- Long press for back navigation  
- Context-aware button functions  
- Visual feedback for disabled states

### Universal Hardware Support  
- Auto-detection of Fire or CoreS3  
- UI optimized per platform  
- Single unified codebase

---

## Supported Devices

| Device            | Input Method       | Special Features                          |
|------------------|--------------------|-------------------------------------------|
| **M5Stack Fire** | Physical Buttons   | Classic 3-button operation with help text |
| **M5Stack CoreS3** | Touchscreen       | Touch UI with visual feedback             |

---

## Timer Modes

### Game Timer  
- Each player starts with the same time pool  
- Countdown only runs during active player's turn  
- Overtime is tracked in red  
- Ideal for turn-based strategy games

### Turn Timer  
- Each turn starts with the same time  
- Timer resets on player switch  
- Suitable for speed games or high-pressure play  
- Ensures consistent pace

---

## Complete User Journey

### 1. Sound Setup
Choose preferred sound behavior:
- **Sound ON**: Full feedback with alerts
- **Sound OFF**: Silent operation

### 2. Mode Selection
Pick your preferred timer mode:
- **Game Timer**: Per-player time pools
- **Turn Timer**: Fixed time per turn

### 3. Time Configuration
- MM:SS format (max 59:59)
- Digit-by-digit editing with blinking cursor
- Live preview during configuration

### 4. Active Timing
- Auto detection via IMU
- Visual player indicators
- Tracks overtime and triggers alerts

---

## Operation Reference

### M5Stack Fire (Physical Buttons)

| Screen              | A Button       | B Button         | C Button       |
|---------------------|----------------|------------------|----------------|
| **Sound Setup**     | Toggle sound   | Confirm option   | -              |
| **Mode Selection**  | Toggle mode    | Confirm mode     | -              |
| **Time Setting**    | +1 (increment) | Start timer      | Next digit     |
| **Timer (Stopped)** | Back to setup  | Start/Resume     | -              |
| **Timer (Running)** | *Disabled*     | Pause            | -              |

**Special:**  
- **Hold C** (in setup): Return to mode selection

### M5Stack CoreS3 (Touch Interface)

| Screen              | Left Touch     | Center Touch     | Right Touch     |
|---------------------|----------------|------------------|------------------|
| **Sound Setup**     | Toggle sound   | Confirm option   | -                |
| **Mode Selection**  | Toggle mode    | Confirm mode     | -                |
| **Time Setting**    | +1             | Start            | Next             |
| **Timer (Stopped)** | Back           | Start            | -                |
| **Timer (Running)** | *Reset*        | Pause            | -                |

**Special:**  
- **Hold right side** (in setup): Return to mode selection  
- Touch areas are clearly defined with visual boundaries

---

## Orientation Mapping

| Position                    | Player    | IMU Axis       | Use Case           |
|----------------------------|-----------|----------------|---------------------|
| Front edge facing player   | Player 1  | Y-axis positive | Standard start      |
| Right edge facing player   | Player 2  | X-axis positive | Clockwise rotation  |
| Back edge facing player    | Player 3  | Y-axis negative | Opponent side       |
| Left edge facing player    | Player 4  | X-axis negative | Counter-clockwise   |
| Face-up (flat position)    | Player 5  | Z-axis positive | Center/Spectator    |

> Face-down Detection: Automatically pauses the timer and turns off the screen to save power.

---

## Setup Guide

### Requirements
- Arduino IDE 1.8.19+
- M5Stack board definitions
- M5Unified library

### Installation

1. **Configure Arduino IDE**
```

File > Preferences > Additional Board Manager URLs:
[https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package\_m5stack\_index.json](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json)

```

2. **Install Board Support**
```

Tools > Board > Board Manager
Search: "M5Stack" → Install

```

3. **Install Required Library**
```

Tools > Manage Libraries
Search: "M5Unified" → Install latest version

````

4. **Select Your Board**
- **Fire**: Tools > Board > M5Stack-Fire  
- **CoreS3**: Tools > Board > M5Stack-CoreS3  

5. **Upload the Program**
- Open `M5Cube.ino`
- Connect device via USB-C
- Select correct COM port
- Upload

---

## Sound System

### Alert Schedule

| Event           | Tone     | Duration |
|------------------|----------|-----------|
| 10 seconds left  | Mid tone | 200 ms    |
| 5–1 seconds left | Low tone | 200 ms    |
| Timeout          | Low tone | 300 ms    |
| Player switch    | High tone| 50 ms     |

> Sound can be disabled entirely during setup—ideal for quiet environments.

---

## Power Management

- **CPU Frequency**: Reduced to 80MHz (from 240MHz)  
- **Display Brightness**: Set to 50%  
- **Auto Sleep**: Triggered by face-down detection  
- **Partial Screen Updates**: Save power  
- **IMU Sampling**: 20ms (active), 100ms (paused)  

**Battery Monitoring:**
- Real-time level display  
- Green: >50%, Yellow: 20–50%, Red: <20%  
- "+" prefix when charging  
- Updates every 5 seconds  

---

## Advanced Features

- **Context-aware controls**  
- **Visual feedback** for invalid actions  
- **Memory optimization** with static buffers  
- **Accurate countdown** (1s resolution)  
- **Overtime tracking** (in red)  
- **Supports up to 5 players**  
- **State persistence during switch**

---

## Customization Constants

```cpp
// Orientation detection
const float ACCEL_THRESHOLD = 0.9f;

// Warning timings
const int WARNING_TIME_10SEC = 10;
const int WARNING_TIME_5SEC = 5;

// Sound frequencies
const int TONE_FREQ_HIGH = 2800;
const int TONE_FREQ_MID = 1800;
const int TONE_FREQ_LOW = 1000;

// Display settings
const int BUTTON_TEXT_SIZE = 2.5;
const int BLINK_INTERVAL = 400;
````

---

## Roadmap

### Upcoming Features

* Additional timer modes like those in DGT CUBE

### Hardware Expansion

* Compatibility with **M5Stack Basic**
* External orientation sensors
* External display support

---

## Technical Specs

| Metric             | Value                    |
| ------------------ | ------------------------ |
| Orientation Detect | < 50ms                   |
| Timer Accuracy     | ±1 sec per hour          |
| Battery Life       | 8–12 hours (typical use) |
| Memory Usage       | < 50% of available RAM   |

---

## License

Released under the MIT License.
Free for personal, educational, and commercial use.

**M5Cube** – A professional timing solution for serious board gamers.
