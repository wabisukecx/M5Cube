#include <M5Unified.h>

// Constants
const float ACCEL_THRESHOLD = 0.9f;
const float ACCEL_THRESHOLD_BACK = -0.9f;
const int TONE_FREQ_HIGH = 2800;
const int TONE_FREQ_LOW = 1000;
const int TONE_DURATION_LONG = 100;
const int TONE_DURATION_WARNING = 100;
const int TIMER_INTERVAL = 1000;
const int BLINK_INTERVAL = 400;
const int IMU_UPDATE_INTERVAL = 20; // IMU update interval (ms)
const int BATTERY_UPDATE_INTERVAL = 5000; // Battery display update interval (ms)
const int PAUSE_IMU_INTERVAL = 100; // IMU update interval during pause (ms)
const int BUTTON_TEXT_SIZE = 2.5; // Touch button text size (adjustable for CoreS3)
const int BUTTON_MARGIN = 8; // Button margin around text (for CoreS3)

// Color definitions
const uint32_t COLOR_WHITE = 0xFFFFFF;
const uint32_t COLOR_RED = 0xFF0000;
const uint32_t COLOR_BLUE = 0x000066;
const uint32_t COLOR_GREEN = 0x00FF00;
const uint32_t COLOR_YELLOW = 0xFFFF00;

// Device type detection
enum DeviceType {
  DEVICE_FIRE,
  DEVICE_CORES3,
  DEVICE_UNKNOWN
};
DeviceType deviceType = DEVICE_UNKNOWN;

// Global variables
int timeList[5] = {0, 0, 0, 0, 0};
int negativeList[5] = {0, 0, 0, 0, 0};
int currentFace = 4; // Current displayed face (0-4)
bool statusFlg = false;
bool startFlg = false;
bool pauseFlg = false;
int updatePosition = 0;
int dig01 = 0, dig02 = 0, dig03 = 0, dig04 = 0;
int initDig01 = 0, initDig02 = 0, initDig03 = 0, initDig04 = 0;

unsigned long lastTimerUpdate = 0;
unsigned long lastBlinkUpdate = 0;
unsigned long lastIMUUpdate = 0; // IMU update management
unsigned long lastBatteryUpdate = 0; // Battery display update management
bool blinkState = true;
bool warningPlayed = false; // Warning sound flag
bool needsFullRedraw = true; // Full redraw flag
bool needsBatteryUpdate = true; // Battery display update flag
bool isDisplayOn = true; // Display ON/OFF flag

// IMU data
float accelX, accelY, accelZ;

// Function prototypes
void detectDeviceType();
void updateTimer(int faceIndex);
void drawTimerDisplay();
void drawTimerDisplayFull(); // Full screen drawing
void updateTimerOnly(); // Timer area only update
void drawSetupMode();
void drawSetupModeFull(); // Setup screen full drawing
void updateSetupTimerOnly(); // Setup screen timer area only update
void handleButtons();
void handlePhysicalButtons();
void handleTouchButtons(); // CoreS3 only
void updateIMU();
int detectOrientation();
void playTone(int frequency, int duration);
void resetAllTimers();
void incrementDigit();
void nextDigitPosition();
String formatTime(int dig1, int dig2, int dig3, int dig4);
void drawBatteryInfo(); // Battery info display
void turnOffDisplay(); // Display sleep function
void turnOnDisplay(); // Display wake function
void drawTouchButtons(bool inSetupMode); // Touch button UI display (CoreS3 only)
void drawFireButtonHelp(bool inSetupMode); // Fire button help display
int getButtonHeight(); // Calculate button height based on text size (CoreS3 only)

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  // Detect device type
  detectDeviceType();
  
  // Power saving settings
  M5.Display.setBrightness(128); // Set brightness to 50% (0-255)
  setCpuFrequencyMhz(80); // Set CPU frequency to 80MHz (default 240MHz)
  
  M5.Display.begin();
  M5.Display.fillScreen(COLOR_BLUE);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(middle_center);
  
  M5.Imu.begin();
  M5.Speaker.begin();
  
  // Initial display
  currentFace = 4; // P5 face (setup face)
  needsFullRedraw = true;
  needsBatteryUpdate = true;
  isDisplayOn = true;
  lastBatteryUpdate = millis();
  drawSetupModeFull();
}

void loop() {
  M5.update();
  
  // IMU update frequency control for power saving
  unsigned long currentTime = millis();
  bool shouldUpdateIMU = false;
  
  if (pauseFlg) {
    // Low frequency IMU update during pause
    if (currentTime - lastIMUUpdate >= PAUSE_IMU_INTERVAL) {
      shouldUpdateIMU = true;
      lastIMUUpdate = currentTime;
    }
  } else {
    // High frequency IMU update during normal operation
    if (currentTime - lastIMUUpdate >= IMU_UPDATE_INTERVAL) {
      shouldUpdateIMU = true;
      lastIMUUpdate = currentTime;
    }
  }
  
  if (shouldUpdateIMU) {
    updateIMU();
  }
  
  handleButtons();
  
  // Battery display update frequency control
  if (currentTime - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
    needsBatteryUpdate = true;
    lastBatteryUpdate = currentTime;
  }
  
  // Timer operation processing
  if (statusFlg && currentTime - lastTimerUpdate >= TIMER_INTERVAL) {
    lastTimerUpdate = currentTime;
    
    int orientation = detectOrientation();
    if (orientation >= 0 && orientation != currentFace) {
      // Face switching
      currentFace = orientation;
      needsFullRedraw = true;
      needsBatteryUpdate = true;
      playTone(TONE_FREQ_HIGH, TONE_DURATION_LONG);
    }
    
    if (orientation >= 0) {
      // Recovery from face-down state
      if (pauseFlg && !isDisplayOn) {
        turnOnDisplay();
        needsFullRedraw = true;
        needsBatteryUpdate = true;
      }
      
      updateTimer(orientation);
      
      // Execute drawing process only when display is ON
      if (isDisplayOn) {
        if (needsFullRedraw) {
          drawTimerDisplayFull();
          needsFullRedraw = false;
          needsBatteryUpdate = false;
        } else {
          updateTimerOnly();
          if (needsBatteryUpdate) {
            drawBatteryInfo();
            needsBatteryUpdate = false;
          }
        }
      }
      pauseFlg = false;
    } else if (accelZ <= ACCEL_THRESHOLD_BACK && pauseFlg == false) {
      // Face-down state processing
      playTone(TONE_FREQ_LOW, TONE_DURATION_WARNING);
      pauseFlg = true;
      if (isDisplayOn) {
        turnOffDisplay();
      }
    }
  }
  
  // Blinking process in setup mode
  if (!startFlg) {
    if (currentTime - lastBlinkUpdate >= BLINK_INTERVAL) {
      lastBlinkUpdate = currentTime;
      blinkState = !blinkState;
      
      if (isDisplayOn) {
        if (needsFullRedraw) {
          drawSetupModeFull();
          needsFullRedraw = false;
          needsBatteryUpdate = false;
        } else {
          updateSetupTimerOnly();
          if (needsBatteryUpdate) {
            drawBatteryInfo();
            needsBatteryUpdate = false;
          }
        }
      }
    }
  }
  
  delay(10); // Increased from 2ms to 10ms for power saving
}

void detectDeviceType() {
  // Device type detection based on M5Unified
  switch (M5.getBoard()) {
    case lgfx::boards::board_M5Stack:
    case lgfx::boards::board_M5Tough:
      deviceType = DEVICE_FIRE;
      break;
    case lgfx::boards::board_M5StackCoreS3:
    case lgfx::boards::board_M5StackCoreS3SE:
      deviceType = DEVICE_CORES3;
      break;
    default:
      deviceType = DEVICE_UNKNOWN;
      // Fallback: try to detect touch capability
      if (M5.Touch.isEnabled()) {
        deviceType = DEVICE_CORES3;
      } else {
        deviceType = DEVICE_FIRE;
      }
      break;
  }
}

void updateTimer(int faceIndex) {
  int totalTime = timeList[faceIndex];
  
  if (negativeList[faceIndex] != 1) {
    if (totalTime > 0) {
      dig01 = totalTime / 600;
      int dig01Mod = totalTime % 600;
      dig02 = dig01Mod / 60;
      int dig02Mod = dig01Mod % 60;
      dig03 = dig02Mod / 10;
      dig04 = dig02Mod % 10;
      timeList[faceIndex] = totalTime - 1;
    } else {
      // When totalTime == 0, display 00:00 in white
      dig01 = dig02 = dig03 = dig04 = 0;
      negativeList[faceIndex] = 1;  // Enter overtime mode from next time
      timeList[faceIndex] = 1;      // Start from 1 second overtime next time
    }
  } else {
    // Overtime processing (1 second, 2 seconds, 3 seconds...)
    dig01 = totalTime / 600;
    int dig01Mod = totalTime % 600;
    dig02 = dig01Mod / 60;
    int dig02Mod = dig01Mod % 60;
    dig03 = dig02Mod / 10;
    dig04 = dig02Mod % 10;
    timeList[faceIndex] = totalTime + 1;
  }
}

void drawTimerDisplayFull() {
  M5.Display.fillScreen(COLOR_BLUE);
  
  // Set screen rotation according to face
  switch (currentFace) {
    case 0: // P1 face - 0 degrees (front)
      M5.Display.setRotation(1);
      break;
    case 1: // P2 face - 90 degrees (right rotation)
      M5.Display.setRotation(2);
      break;
    case 2: // P3 face - 180 degrees (upside down)
      M5.Display.setRotation(3);
      break;
    case 3: // P4 face - 270 degrees (left rotation)
      M5.Display.setRotation(0);
      break;
    case 4: // P5 face - 0 degrees (front)
      M5.Display.setRotation(1);
      break;
  }
  
  // Get screen size after rotation
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int centerX = screenW / 2;
  int centerY = screenH / 2;
  
  // Battery info display (top right)
  drawBatteryInfo();
  
  // Current face display
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(top_center);
  M5.Display.drawString("Player:" + String(currentFace + 1), centerX, 20);
  
  // Determine timer display color
  uint32_t timerColor = (negativeList[currentFace] == 1) ? COLOR_RED : COLOR_WHITE;
  
  // Timer display (large, center)
  M5.Display.setTextSize(6);
  M5.Display.setTextColor(timerColor);
  M5.Display.setTextDatum(middle_center);
  String timeStr = formatTime(dig01, dig02, dig03, dig04);
  M5.Display.drawString(timeStr, centerX, centerY);
  
  // Status display
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(bottom_center);
  String status = statusFlg ? "RUNNING" : "STOPPED";
  M5.Display.drawString(status, centerX, screenH - 60);
  
  // Device-specific UI elements
  if (deviceType == DEVICE_CORES3) {
    // Draw touch buttons for CoreS3
    drawTouchButtons(false);
  } else {
    // Draw button help for Fire
    drawFireButtonHelp(false);
  }
}

void updateTimerOnly() {
  // Get screen size after rotation
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int centerX = screenW / 2;
  int centerY = screenH / 2;
  
  // Clear timer display area background (partial)
  int timerAreaWidth = 200;
  int timerAreaHeight = 60;
  M5.Display.fillRect(centerX - timerAreaWidth/2, centerY - timerAreaHeight/2, 
                      timerAreaWidth, timerAreaHeight, COLOR_BLUE);
  
  // Determine timer display color
  uint32_t timerColor = (negativeList[currentFace] == 1) ? COLOR_RED : COLOR_WHITE;
  
  // Redraw timer display only
  M5.Display.setTextSize(6);
  M5.Display.setTextColor(timerColor);
  M5.Display.setTextDatum(middle_center);
  String timeStr = formatTime(dig01, dig02, dig03, dig04);
  M5.Display.drawString(timeStr, centerX, centerY);
}

void drawTimerDisplay() {
  // For compatibility, call full drawing
  drawTimerDisplayFull();
}

void drawSetupModeFull() {
  M5.Display.fillScreen(COLOR_BLUE);
  M5.Display.setRotation(1); // Setup screen always faces front
  
  // Battery info display (top right)
  drawBatteryInfo();
  
  // Setup mode display
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(top_center);
  M5.Display.drawString("SETUP", 160, 30);
  
  // Time setting display
  M5.Display.setTextSize(6);
  String timeStr = formatTime(dig01, dig02, dig03, dig04);
  
  // Blinking process
  if (!blinkState) {
    switch (updatePosition) {
      case 0: timeStr[0] = ' '; break;  // Minutes tens place
      case 1: timeStr[1] = ' '; break;  // Minutes ones place
      case 2: timeStr[3] = ' '; break;  // Seconds tens place
      case 3: timeStr[4] = ' '; break;  // Seconds ones place
    }
  }
  
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(timeStr, 160, 120);

  // Device-specific UI elements
  if (deviceType == DEVICE_CORES3) {
    // Draw touch buttons for CoreS3
    drawTouchButtons(true);
  } else {
    // Draw button help for Fire
    drawFireButtonHelp(true);
  }
}

void updateSetupTimerOnly() {
  // Clear time display area background (partial)
  int timerAreaWidth = 240;
  int timerAreaHeight = 50;
  M5.Display.fillRect(160 - timerAreaWidth/2, 120 - timerAreaHeight/2, 
                      timerAreaWidth, timerAreaHeight, COLOR_BLUE);
  
  // Redraw time setting display only
  M5.Display.setTextSize(6);
  String timeStr = formatTime(dig01, dig02, dig03, dig04);
  
  // Blinking process
  if (!blinkState) {
    switch (updatePosition) {
      case 0: timeStr[0] = ' '; break;  // Minutes tens place
      case 1: timeStr[1] = ' '; break;  // Minutes ones place
      case 2: timeStr[3] = ' '; break;  // Seconds tens place
      case 3: timeStr[4] = ' '; break;  // Seconds ones place
    }
  }
  
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(timeStr, 160, 120);
}

void drawSetupMode() {
  // For compatibility, call full drawing
  drawSetupModeFull();
}

void drawBatteryInfo() {
  // Get current screen size
  int screenW = M5.Display.width();
  
  // Get battery information
  bool isCharging = M5.Power.isCharging();
  int batteryLevel = M5.Power.getBatteryLevel();
  
  // Clear previous display (partial update for power saving)
  M5.Display.fillRect(screenW - 50, 5, 45, 20, COLOR_BLUE);
  
  // Change color according to battery level
  uint32_t batteryColor = COLOR_WHITE;
  if (batteryLevel <= 20) {
    batteryColor = COLOR_RED;
  } else if (batteryLevel <= 50) {
    batteryColor = COLOR_YELLOW;
  } else {
    batteryColor = COLOR_GREEN;
  }
  
  // Change color when charging
  if (isCharging) {
    batteryColor = COLOR_GREEN;
  }
  
  // Battery display (top right)
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(batteryColor);
  M5.Display.setTextDatum(top_right);
  
  // String generation optimization
  static String batteryText; // Static variable for string reuse
  batteryText = String(batteryLevel) + "%";
  if (isCharging) {
    batteryText = "+" + batteryText;
  }
  
  M5.Display.drawString(batteryText, screenW - 5, 5);
}

String formatTime(int dig1, int dig2, int dig3, int dig4) {
  return String(dig1) + String(dig2) + ":" + String(dig3) + String(dig4);
}

void handleButtons() {
  // When display is off, restore screen with any button operation
  bool anyButtonPressed = false;
  
  // Check physical buttons
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    anyButtonPressed = true;
  }
  
  // Check touch buttons for CoreS3
  if (deviceType == DEVICE_CORES3 && M5.Touch.getCount() > 0) {
    auto touch = M5.Touch.getDetail(0);
    if (touch.wasPressed()) {
      anyButtonPressed = true;
    }
  }
  
  if (!isDisplayOn && anyButtonPressed) {
    turnOnDisplay();
    needsFullRedraw = true;
    needsBatteryUpdate = true;
    // Button processing will be executed in next loop (prioritize screen restoration)
    return;
  }
  
  // Handle device-specific button input
  handlePhysicalButtons();
  if (deviceType == DEVICE_CORES3) {
    handleTouchButtons();
  }
}

void handlePhysicalButtons() {
  bool leftPressed = M5.BtnA.wasPressed();
  bool centerPressed = M5.BtnB.wasPressed();
  bool rightPressed = M5.BtnC.wasPressed();
  
  // Center button - Start/Stop function
  if (centerPressed) {
    if (!statusFlg) {
      // Start timer
      statusFlg = true;
      startFlg = true;
      warningPlayed = false;
      lastTimerUpdate = millis();
      needsFullRedraw = true;
      needsBatteryUpdate = true;
      
      // Detect current orientation at start and set face
      int orientation = detectOrientation();
      if (orientation >= 0) {
        currentFace = orientation;
      } else {
        currentFace = 4; // Default to P5 face
      }
    } else {
      // Stop timer
      statusFlg = false;
      needsFullRedraw = true;
      needsBatteryUpdate = true;
      if (isDisplayOn) {
        drawTimerDisplayFull();
      }
    }
  }
  
  // Left button - Reset/Increment function
  if (leftPressed) {
    if (!startFlg) {
      // Setup mode: increment value
      incrementDigit();
    } else if (!statusFlg) {
      // Stopped: reset
      resetAllTimers();
    }
  }
  
  // Right button - Next digit function
  if (rightPressed) {
    if (!startFlg) {
      // Setup mode: next digit
      nextDigitPosition();
    }
  }
}

void handleTouchButtons() {
  // Handle touch screen input for CoreS3
  bool leftPressed = false;
  bool centerPressed = false;
  bool rightPressed = false;
  
  // Check for touch events
  if (M5.Touch.getCount() > 0) {
    auto touch = M5.Touch.getDetail(0);
    if (touch.wasPressed()) {
      int touchX = touch.x;
      int touchY = touch.y;
      int screenW = M5.Display.width();
      int screenH = M5.Display.height();
      int buttonHeight = getButtonHeight(); // Use calculated button height
      int buttonY = screenH - buttonHeight;
      int buttonWidth = screenW / 3;
      
      // Check if touch is in button area
      if (touchY >= buttonY && touchY <= screenH) {
        if (touchX >= 0 && touchX < buttonWidth) {
          leftPressed = true;
        } else if (touchX >= buttonWidth && touchX < buttonWidth * 2) {
          centerPressed = true;
        } else if (touchX >= buttonWidth * 2 && touchX < screenW) {
          rightPressed = true;
        }
      }
    }
  }
  
  // Process touch button presses (same logic as physical buttons)
  if (centerPressed) {
    if (!statusFlg) {
      // Start timer
      statusFlg = true;
      startFlg = true;
      warningPlayed = false;
      lastTimerUpdate = millis();
      needsFullRedraw = true;
      needsBatteryUpdate = true;
      
      // Detect current orientation at start and set face
      int orientation = detectOrientation();
      if (orientation >= 0) {
        currentFace = orientation;
      } else {
        currentFace = 4; // Default to P5 face
      }
    } else {
      // Stop timer
      statusFlg = false;
      needsFullRedraw = true;
      needsBatteryUpdate = true;
      if (isDisplayOn) {
        drawTimerDisplayFull();
      }
    }
  }
  
  if (leftPressed) {
    if (!startFlg) {
      // Setup mode: increment value
      incrementDigit();
    } else if (!statusFlg) {
      // Stopped: reset
      resetAllTimers();
    }
  }
  
  if (rightPressed) {
    if (!startFlg) {
      // Setup mode: next digit
      nextDigitPosition();
    }
  }
}

void updateIMU() {
  auto imu_update = M5.Imu.update();
  if (imu_update) {
    auto imu_data = M5.Imu.getImuData();
    accelX = imu_data.accel.x;
    accelY = imu_data.accel.y;
    accelZ = imu_data.accel.z;
  }
}

int detectOrientation() {
  if (accelY >= ACCEL_THRESHOLD) return 0; // P1 face (Y-axis positive)
  if (accelX >= ACCEL_THRESHOLD) return 1; // P2 face (X-axis positive)
  if (accelY <= -ACCEL_THRESHOLD) return 2; // P3 face (Y-axis negative)
  if (accelX <= -ACCEL_THRESHOLD) return 3; // P4 face (X-axis negative)
  if (accelZ >= ACCEL_THRESHOLD) return 4; // P5 face (Z-axis positive)
  return -1; // No match
}

void playTone(int frequency, int duration) {
  M5.Speaker.tone(frequency, duration);
}

void resetAllTimers() {
  statusFlg = false;
  startFlg = false;
  currentFace = 4; // Return to P5 face
  warningPlayed = false;
  needsFullRedraw = true;
  needsBatteryUpdate = true;
  
  // Restore screen if it was off
  if (!isDisplayOn) {
    turnOnDisplay();
  }
  
  dig01 = initDig01;
  dig02 = initDig02;
  dig03 = initDig03;
  dig04 = initDig04;
  
  int totalTime = dig01 * 600 + dig02 * 60 + dig03 * 10 + dig04;
  for (int i = 0; i < 5; i++) {
    timeList[i] = totalTime;
    negativeList[i] = 0;
  }
  
  drawSetupModeFull();
}

void incrementDigit() {
  switch (updatePosition) {
    case 0:
      dig01 = (dig01 < 5) ? dig01 + 1 : 0;
      break;
    case 1:
      dig02 = (dig02 < 9) ? dig02 + 1 : 0;
      break;
    case 2:
      dig03 = (dig03 < 5) ? dig03 + 1 : 0;
      break;
    case 3:
      dig04 = (dig04 < 9) ? dig04 + 1 : 0;
      break;
  }
  
  // Save setting values
  initDig01 = dig01;
  initDig02 = dig02;
  initDig03 = dig03;
  initDig04 = dig04;
  
  // Apply to all timers
  int totalTime = dig01 * 600 + dig02 * 60 + dig03 * 10 + dig04;
  for (int i = 0; i < 5; i++) {
    timeList[i] = totalTime;
    negativeList[i] = 0;
  }
}

void turnOffDisplay() {
  if (isDisplayOn) {
    M5.Display.setBrightness(0); // Turn off display
    isDisplayOn = false;
  }
}

void turnOnDisplay() {
  if (!isDisplayOn) {
    M5.Display.setBrightness(128); // Restore display (50% brightness)
    isDisplayOn = true;
  }
}

void drawTouchButtons(bool inSetupMode) {
  // CoreS3 only - Touch button UI
  if (deviceType != DEVICE_CORES3) return;
  
  // Get current screen size
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  
  // Touch button area dimensions (calculated from text size)
  int buttonHeight = getButtonHeight();
  int buttonY = screenH - buttonHeight;
  int buttonWidth = screenW / 3;
  
  // Clear button area
  M5.Display.fillRect(0, buttonY, screenW, buttonHeight, COLOR_BLUE);
  
  // Draw button borders
  M5.Display.drawLine(buttonWidth, buttonY, buttonWidth, screenH, COLOR_WHITE);
  M5.Display.drawLine(buttonWidth * 2, buttonY, buttonWidth * 2, screenH, COLOR_WHITE);
  M5.Display.drawLine(0, buttonY, screenW, buttonY, COLOR_WHITE);
  
  // Button text settings (size based on constant)
  M5.Display.setTextSize(BUTTON_TEXT_SIZE);
  M5.Display.setTextDatum(middle_center);
  
  if (inSetupMode) {
    // Setup mode buttons
    M5.Display.setTextColor(COLOR_WHITE);
    M5.Display.drawString("+1", buttonWidth / 2, buttonY + buttonHeight / 2);
    M5.Display.drawString("START", buttonWidth + buttonWidth / 2, buttonY + buttonHeight / 2);
    M5.Display.drawString("NEXT", buttonWidth * 2 + buttonWidth / 2, buttonY + buttonHeight / 2);
  } else {
    // Timer mode buttons
    if (statusFlg) {
      // Running: Reset button disabled (grayed out)
      M5.Display.setTextColor(COLOR_BLUE); // Dimmed color to indicate disabled
      M5.Display.drawString("(RESET)", buttonWidth / 2, buttonY + buttonHeight / 2);
      M5.Display.setTextColor(COLOR_WHITE);
      M5.Display.drawString("STOP", buttonWidth + buttonWidth / 2, buttonY + buttonHeight / 2);
    } else {
      // Stopped: Reset button enabled
      M5.Display.setTextColor(COLOR_WHITE);
      M5.Display.drawString("RESET", buttonWidth / 2, buttonY + buttonHeight / 2);
      M5.Display.drawString("START", buttonWidth + buttonWidth / 2, buttonY + buttonHeight / 2);
    }
    M5.Display.drawString("", buttonWidth * 2 + buttonWidth / 2, buttonY + buttonHeight / 2); // No function for right button in timer mode
  }
}

void drawFireButtonHelp(bool inSetupMode) {
  // Fire only - Button help text
  if (deviceType != DEVICE_FIRE) return;
  
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int centerX = screenW / 2;
  
  M5.Display.setTextSize(2);
  M5.Display.setTextDatum(bottom_center);
  
  if (inSetupMode) {
    M5.Display.setTextColor(COLOR_WHITE);
    M5.Display.drawString("A:+1  B:Start  C:Next", centerX, 200);
  } else {
    if (statusFlg) {
      // Running: Reset button disabled
      if (currentFace == 1 || currentFace == 3) { // Player2 or Player4
        M5.Display.setTextColor(COLOR_BLUE); // Dimmed color to indicate disabled
        M5.Display.drawString("A:(Reset)", centerX, screenH - 35);
        M5.Display.setTextColor(COLOR_WHITE);
        M5.Display.drawString("B:Stop", centerX, screenH - 15);
      } else {
        M5.Display.setTextColor(COLOR_BLUE); // Dimmed color to indicate disabled
        M5.Display.drawString("A:(Reset)", centerX, screenH - 25);
        M5.Display.setTextColor(COLOR_WHITE);
        M5.Display.drawString("B:Stop", centerX, screenH - 5);
      }
    } else {
      // Stopped: Reset button enabled
      M5.Display.setTextColor(COLOR_WHITE);
      if (currentFace == 1 || currentFace == 3) { // Player2 or Player4
        M5.Display.drawString("A:Reset", centerX, screenH - 35);
        M5.Display.drawString("B:Start", centerX, screenH - 15);
      } else {
        M5.Display.drawString("A:Reset  B:Start", centerX, screenH - 20);
      }
    }
  }
}

int getButtonHeight() {
  // CoreS3 only - Calculate button height based on text size and margin
  if (deviceType != DEVICE_CORES3) return 0;
  
  // Text height is approximately (textSize * 8) pixels
  return (BUTTON_TEXT_SIZE * 8) + (BUTTON_MARGIN * 2);
}

void nextDigitPosition() {
  updatePosition = (updatePosition < 3) ? updatePosition + 1 : 0;
}