# M5Cube - Orientation-Based Multiplayer Timer

This is a multiplayer game timer application designed specifically for the M5Stack platform, inspired by the DGT Cube board game timer. The timer automatically detects device orientation using the built-in IMU sensor to manage up to five different player timers. I originally developed this for tabletop gaming sessions where multiple players need individual countdown timers, and the orientation-based switching eliminates the need for complex menu navigation.

The concept came from noticing the physical similarity between the M5Stack's compact cube-like form factor and the DGT Cube. Both devices use their orientation as a primary interface method, making the M5Stack a natural platform for recreating this intuitive timer concept for board gaming.

While the DGT Cube supports up to 6 players with dedicated displays on each face and uses a special base for pausing, M5Cube adapts this concept for the M5Stack platform with 5 player support, a single rotating display, and power-efficient face-down detection for automatic screen shutoff.

## Background and Development

I created this timer after getting frustrated with managing multiple stopwatches during board game nights. The idea was simple: place the M5Stack device facing different directions, and it would automatically switch to the corresponding player's timer. The device uses its accelerometer to detect which face is pointing up, making it incredibly intuitive to use during gameplay.

The application has been tested exclusively on the M5Stack Fire, so while it should theoretically work on other M5Stack devices with similar hardware, I cannot guarantee compatibility. The M5Stack Fire's 6-axis IMU and speaker are essential for the core functionality.

## Hardware Requirements

You will need an M5Stack Fire device for this application. While the code uses M5Unified library which supports multiple M5Stack variants, I have only verified operation on the Fire model. The application requires several hardware components that are standard on the Fire:

The 6-axis IMU sensor is crucial for orientation detection. Without this, the automatic player switching won't work. The built-in speaker provides audio feedback when switching between players or entering pause mode. The device also needs to be battery powered for portability during gaming sessions.

## Installation and Setup

Setting up the development environment for M5Stack can be tricky if you're new to the platform, so I'll walk through the complete process for both Arduino IDE and PlatformIO options.

### Arduino IDE Setup (Recommended for Beginners)

If you're new to M5Stack development, the Arduino IDE is probably your best starting point. First, download and install the latest Arduino IDE from the official Arduino website. Make sure you get version 1.8.19 or later, as earlier versions may have compatibility issues with the M5Stack board definitions.

Once Arduino IDE is installed, you need to add the M5Stack board definitions. Open Arduino IDE and go to File → Preferences. In the "Additional Board Manager URLs" field, add this URL: `https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json`. If you already have other URLs in this field, separate them with commas.

Next, go to Tools → Board → Boards Manager. Search for "M5Stack" and install the "M5Stack by M5Stack" package. This process might take a few minutes as it downloads all the necessary files. After installation, you should see M5Stack options under Tools → Board → M5Stack Arduino.

For the M5Stack Fire specifically, select "M5Stack-Fire" from the board menu. Set the upload speed to 921600 for faster uploads, though you can use 115200 if you encounter upload issues. The partition scheme should be set to "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)".

### Installing M5Unified Library

The M5Unified library is essential and replaces the older separate M5Stack libraries. In Arduino IDE, go to Tools → Manage Libraries (or use Ctrl+Shift+I). Search for "M5Unified" and install the library by M5Stack. This library provides unified access to all M5Stack device features and is actively maintained, unlike some of the older individual libraries.

Make sure you install the latest version of M5Unified, as older versions may have compatibility issues or missing features. The library will automatically pull in any additional dependencies it needs, so you don't need to install anything else manually.

### Uploading the Code

Download the M5Cube_M5Fire.ino file and open it in your chosen development environment. Connect your M5Stack Fire to your computer using a USB-C cable. Make sure the device is powered on and recognized by your system. On Windows, you might need to install CP210x USB drivers if the device isn't detected.

Before uploading, double-check your board and port settings. The port should show something like "COM3" on Windows or "/dev/ttyUSB0" on Linux. If you don't see any ports listed, try a different USB cable or restart the M5Stack device.

Click the upload button and wait for the process to complete. The first upload might take longer as the development environment compiles all the libraries. If you encounter upload errors, try holding the reset button on the M5Stack while initiating the upload, then release it when the upload progress begins.

After successful upload, the device will automatically restart and boot into setup mode where you can configure your initial timer duration. If the screen remains blank, try pressing the reset button or disconnecting and reconnecting the power.

## How It Works

When you first power on the device, it starts in setup mode where you can configure the timer duration. The display shows "SETUP" at the top with a time in MM:SS format below. Use the three buttons to set your desired countdown time. Button A increments the currently selected digit, Button C moves to the next digit position, and Button B starts the timer.

Once the timer starts, the magic happens through orientation detection. The device continuously monitors its accelerometer to determine which face is pointing upward. Each orientation corresponds to a different player:

Player 1 corresponds to the Y-axis positive direction, which typically means one side edge pointing up. Player 2 uses the X-axis positive direction, another side edge. Player 3 is Y-axis negative, the opposite side from Player 1. Player 4 uses X-axis negative, opposite from Player 2. Player 5 corresponds to Z-axis positive, which is the normal flat position with the screen facing up.

The threshold for detection is set at 0.7g acceleration in any direction. This provides reliable switching while avoiding false triggers from small movements or vibrations during gameplay.

## Power Management Features

One of the key features I implemented is intelligent power management. Gaming sessions can last hours, so battery life was a major concern. The device reduces CPU frequency from the default 240MHz down to 80MHz, which significantly extends battery life without affecting performance for this application.

The screen brightness is set to 50% by default, which provides good visibility while conserving power. The battery percentage is displayed in the top-right corner, with color coding: green for good levels, yellow for moderate, and red for low battery. When the device is charging, it shows a plus sign before the percentage.

The most innovative power feature is automatic screen shutoff when the device is placed face-down. When the Z-axis acceleration drops below -0.7g (indicating the screen is facing down), the device plays a low-frequency warning tone and turns off the display. The timer continues running in the background, but the screen consumes no power. Simply flip the device back over to restore the display and continue timing.

## User Interface and Visual Feedback

The interface adapts to each player orientation by automatically rotating the display. This ensures that no matter which direction you're viewing from, the timer appears right-side-up. Player numbers are clearly displayed at the top of the screen.

Time is shown in large, easy-to-read digits in MM:SS format. When a timer reaches zero, it doesn't stop - instead, it continues counting upward in red text to show overtime. This is particularly useful in games where players might go over their allotted time but still need to track how much extra time they've used.

The status line shows whether the timer is "RUNNING" or "STOPPED". Button help text appears at the bottom, though the layout adjusts for different orientations to ensure readability.

## Timer Logic and Behavior

Each of the five player positions maintains its own independent timer. When you set the initial time in setup mode, this duration is copied to all five player timers. As you switch between orientations during gameplay, each timer continues counting down independently.

When a timer reaches zero, it immediately switches to overtime mode, displaying the elapsed overtime in red. This visual distinction makes it immediately obvious when a player has exceeded their allocated time. The timer never stops automatically - you must manually stop it using Button B.

Resetting the timers returns all five players to the originally configured duration and switches back to setup mode. This is useful for starting a new round or game with the same time limits.

## Technical Implementation Details

The code uses several optimization techniques to balance functionality with performance. IMU updates occur every 20ms during normal operation but reduce to every 100ms when paused to save battery. The display uses partial screen updates whenever possible, only redrawing changed portions rather than the entire screen.

Battery information updates every 5 seconds rather than continuously to reduce processing overhead. The orientation detection algorithm uses simple threshold comparison rather than complex vector math to minimize CPU usage.

Audio feedback uses different tones for different events: a high-frequency beep when switching players and a low-frequency warning when entering pause mode. The speaker settings are configured for clear audio without excessive volume.

## Troubleshooting Common Issues

If the orientation detection seems unreliable, try adjusting the ACCEL_THRESHOLD value in the code. The default 0.7 works well for typical usage, but different handling styles might require tuning. Values between 0.5 and 1.0 usually work best.

Screen flickering or strange behavior often indicates power issues. Make sure the battery is adequately charged, and consider reducing screen brightness further if problems persist. The auto-shutoff feature when face-down helps conserve power for extended gaming sessions.

If the device becomes unresponsive, try pressing any button when the screen is off to wake it up. The power management features sometimes make it appear frozen when it's actually just in power-save mode.

## Customization Options

The code includes several constants that can be modified for different use cases. The orientation thresholds can be adjusted if you find the switching too sensitive or not sensitive enough. The tone frequencies and durations can be changed to suit different preferences or environments.

Color constants are defined at the top of the file for easy modification. You might want different colors for different game types or visibility conditions. The timer intervals can also be adjusted, though I recommend keeping them at 1-second resolution for most gaming applications.

## Limitations and Future Development

Currently, the application only supports minutes and seconds, not hours. For most tabletop games, this range is sufficient, but longer strategy games might benefit from hour support. The maximum timer duration is 59:59, which should cover most gaming scenarios.

The orientation detection works best when the device is relatively stable. Rapid movements or vibrations can cause unwanted switching, though the threshold is set to minimize this issue. In very active gaming environments, you might need to adjust the sensitivity.

Since I've only tested on M5Stack Fire, compatibility with other M5Stack models remains unverified. The M5Unified library should provide good compatibility, but hardware differences might require code adjustments.

## Conclusion

M5Cube transforms the M5Stack Fire into an intuitive multiplayer timer perfect for tabletop gaming. The orientation-based switching eliminates the complexity of menu navigation while providing individual timing for up to five players. Power management features ensure it can last through extended gaming sessions, and the visual feedback makes it easy to see timer status at a glance.

The code is designed to be easily modifiable for different requirements while maintaining the core functionality that makes it useful for gaming. Whether you're timing turns in a strategy game or managing discussion periods in a party game, this timer provides a simple, effective solution.
