/*
 * M5Stack Board Game Clock Timer
 * 
 * A multi-player timer application for M5Stack devices (Fire/CoreS3).
 * Supports both Game Timer (total time per player) and Move Timer (time per move) modes.
 * Features IMU-based orientation detection for automatic player switching.
 * 
 * Compatible devices:
 * - M5Stack Fire (physical buttons)
 * - M5Stack CoreS3 (touch screen)
 */

#include <M5Unified.h>

// =============================================================================
// CONSTANTS AND CONFIGURATION
// =============================================================================

// IMU sensitivity thresholds
const float ACCEL_THRESHOLD = 0.9f;
const float ACCEL_THRESHOLD_BACK = -0.9f;

// Audio frequency and duration settings
const int TONE_FREQ_HIGH = 2800;
const int TONE_FREQ_MID = 1800;
const int TONE_FREQ_LOW = 1000;
const int TONE_DURATION_LONG = 100;
const int TONE_DURATION_WARNING = 100;
const int TONE_DURATION_SHORT = 50;

// Timing intervals (milliseconds)
const int TIMER_INTERVAL = 1000;
const int BLINK_INTERVAL = 400;
const int IMU_UPDATE_INTERVAL = 20;
const int BATTERY_UPDATE_INTERVAL = 5000;
const int PAUSE_IMU_INTERVAL = 100;

// UI layout constants
const int BUTTON_TEXT_SIZE = 2.5;
const int BUTTON_MARGIN = 8;

// Warning sound timing (seconds)
const int WARNING_TIME_10SEC = 10;
const int WARNING_TIME_5SEC = 5;
const int WARNING_TIME_4SEC = 4;
const int WARNING_TIME_3SEC = 3;
const int WARNING_TIME_2SEC = 2;
const int WARNING_TIME_1SEC = 1;

// Time calculation constants
const int SECONDS_PER_MINUTE = 60;
const int SECONDS_PER_TEN_MINUTES = 600;
const int MINUTES_PER_HOUR = 60;
const int MAX_PLAYERS = 5;
const int MAX_DIGIT_TENS_MINUTES = 5;
const int MAX_DIGIT_ONES = 9;
const int MAX_DIGIT_TENS_SECONDS = 5;

// String buffer sizes
const int TIME_BUFFER_SIZE = 6;
const int BATTERY_BUFFER_SIZE = 8;
const int TITLE_BUFFER_SIZE = 16;

// Color definitions
const uint32_t COLOR_WHITE = 0xFFFFFF;
const uint32_t COLOR_RED = 0xFF0000;
const uint32_t COLOR_BLUE = 0x000066;
const uint32_t COLOR_GREEN = 0x00FF00;
const uint32_t COLOR_YELLOW = 0xFFFF00;

// Long press detection
const unsigned long LONG_PRESS_DURATION = 1000;

// レイアウト用定数
const int SCREEN_CENTER_X = 160;
const int SCREEN_CENTER_Y = 120;
const int TITLE_Y = 30;
const int OPTION1_Y = 90;
const int OPTION2_Y = 130;
const int TIMER_Y = 120;
const int TIMER_AREA_WIDTH = 200;
const int TIMER_AREA_HEIGHT = 60;

// レイアウト座標取得関数
inline int getScreenCenterX() { return M5.Display.width() / 2; }
inline int getScreenCenterY() { return M5.Display.height() / 2; }
inline int getTitleY() { return TITLE_Y; }
inline int getOptionY(int idx) { return OPTION1_Y + (idx * (OPTION2_Y - OPTION1_Y)); }
inline int getTimerY() { return TIMER_Y; }

// =============================================================================
// DATA STRUCTURES AND ENUMS
// =============================================================================

// Display area structure for UI layout calculations
struct DisplayArea {
  int x, y, width, height;
};

// Device type detection for hardware-specific UI
enum DeviceType {
  DEVICE_FIRE,     // M5Stack Fire (physical buttons)
  DEVICE_CORES3,   // M5Stack CoreS3 (touch screen)
  DEVICE_UNKNOWN
};

// Timer operation modes
enum TimerMode {
  GAME_TIMER,  // Traditional chess clock (total time per player)
  MOVE_TIMER   // Move-based timer (fixed time per move)
};

// Application screen states
enum ScreenState {
  SCREEN_SOUND_SELECT, // Sound on/off configuration
  SCREEN_MODE_SELECT,  // Timer mode selection
  SCREEN_SETUP,        // Time setting configuration
  SCREEN_TIMER         // Active timer operation
};

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

// Device and configuration
DeviceType deviceType = DEVICE_UNKNOWN;
bool soundEnabled = true;
int soundSelectPosition = 0;
TimerMode currentTimerMode = GAME_TIMER;
ScreenState currentScreen = SCREEN_SOUND_SELECT;
int modeSelectPosition = 0;

// Timer state management
int timeList[MAX_PLAYERS] = {0, 0, 0, 0, 0};
int negativeList[MAX_PLAYERS] = {0, 0, 0, 0, 0};
int moveTimeRemaining = 0;
int currentFace = 4;
bool statusFlg = false;
bool startFlg = false;
bool pauseFlg = false;

// Time setting digits
int updatePosition = 0;
int dig01 = 0, dig02 = 0, dig03 = 0, dig04 = 0;
int initDig01 = 0, initDig02 = 0, initDig03 = 0, initDig04 = 0;
int moveTimerDig01 = 0, moveTimerDig02 = 0, moveTimerDig03 = 0, moveTimerDig04 = 0;

// Timing control
unsigned long lastTimerUpdate = 0;
unsigned long lastBlinkUpdate = 0;
unsigned long lastIMUUpdate = 0;
unsigned long lastBatteryUpdate = 0;

// UI state management
bool blinkState = true;
bool warningPlayed = false;
bool needsFullRedraw = true;
bool needsBatteryUpdate = true;
bool isDisplayOn = true;
bool isPlayingWarningSound = false;

// Warning sound state tracking (per player)
bool warning10secPlayed[MAX_PLAYERS] = {false, false, false, false, false};
bool warning5secPlayed[MAX_PLAYERS] = {false, false, false, false, false};
bool warning4secPlayed[MAX_PLAYERS] = {false, false, false, false, false};
bool warning3secPlayed[MAX_PLAYERS] = {false, false, false, false, false};
bool warning2secPlayed[MAX_PLAYERS] = {false, false, false, false, false};
bool warning1secPlayed[MAX_PLAYERS] = {false, false, false, false, false};

// Button state tracking
unsigned long btnCPressTime = 0;
bool btnCLongPressed = false;
unsigned long touchRightPressTime = 0;
bool touchRightLongPressed = false;
bool touchRightCurrentlyPressed = false;

// IMU sensor data
float accelX, accelY, accelZ;

// Memory-optimized string buffers
static char timeBuffer[TIME_BUFFER_SIZE];
static char batteryBuffer[BATTERY_BUFFER_SIZE];
static char titleBuffer[TITLE_BUFFER_SIZE];

// =============================================================================
// FUNCTION PROTOTYPES
// =============================================================================

// Core system functions
void detectDeviceType();
void updateGameTimer(int faceIndex);
void updateMoveTimer(int faceIndex);
void checkTimeWarnings(int remainingSeconds, int playerIndex);
void resetWarningFlags(int playerIndex = -1);

// Screen rendering functions
void drawSoundSelectScreenFull();
void updateSoundSelectOnly();
void drawModeSelectScreenFull();
void updateModeSelectOnly();
void drawSetupModeFull();
void updateSetupTimerOnly();
void drawTimerDisplayFull();
void updateTimerOnly();
void drawTimerDisplay();
void drawSetupMode();
void drawSelectMenu(const char* title, const char* options[], int optionCount, int selectedIdx);

// Input handling
void handleButtons();
void handlePhysicalButtons();
void handleTouchButtons();

// IMU and orientation
void updateIMU();
int detectOrientation();

// Audio output
void playTone(int frequency, int duration);

// Navigation and state management
void resetToModeSelect();
void resetToSetup();
void resetCurrentTimer();
void incrementDigit();
void nextDigitPosition();
void initializeTimers();
void switchToNextPlayer();

// Display management
void drawBatteryInfo();
void turnOffDisplay();
void turnOnDisplay();
void drawTouchButtons();
void drawFireButtonHelp();
int getButtonHeight();

// Utility functions
bool isValidPlayerIndex(int playerIndex);
void calculateTimeDigits(int totalSeconds, int& dig1, int& dig2, int& dig3, int& dig4);
DisplayArea getTimerDisplayArea();
DisplayArea getBatteryDisplayArea();
DisplayArea getButtonArea();
void formatTime(int dig1, int dig2, int dig3, int dig4, char* buffer);
void formatBatteryText(int batteryLevel, bool isCharging, char* buffer);
void formatModeTitle(TimerMode mode, char* buffer);

// =============================================================================
// MAIN SETUP AND LOOP
// =============================================================================

void setup() {
  // Initialize M5Stack with default configuration
  auto cfg = M5.config();
  M5.begin(cfg);
  
  // Detect hardware type for UI adaptation
  detectDeviceType();
  
  // Power optimization settings
  M5.Display.setBrightness(128); // 50% brightness for battery life
  setCpuFrequencyMhz(80);        // Reduce CPU frequency from 240MHz to 80MHz
  
  // Initialize display with blue background
  M5.Display.begin();
  M5.Display.fillScreen(COLOR_BLUE);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(middle_center);
  
  // Initialize peripherals
  M5.Imu.begin();
  M5.Speaker.begin();
  
  // Set initial application state
  currentScreen = SCREEN_SOUND_SELECT;
  needsFullRedraw = true;
  needsBatteryUpdate = true;
  isDisplayOn = true;
  lastBatteryUpdate = millis();
  
  // Render initial screen
  drawSoundSelectScreenFull();
}

void loop() {
  M5.update();
  
  unsigned long currentTime = millis();
  
  // IMU update frequency control for power efficiency
  bool shouldUpdateIMU = false;
  int imuInterval = pauseFlg ? PAUSE_IMU_INTERVAL : IMU_UPDATE_INTERVAL;
  
  if (currentTime - lastIMUUpdate >= imuInterval) {
    shouldUpdateIMU = true;
    lastIMUUpdate = currentTime;
  }
  
  // Update IMU only during timer operation
  if (shouldUpdateIMU && currentScreen == SCREEN_TIMER) {
    updateIMU();
  }
  
  // Process user input
  handleButtons();
  
  // Schedule battery display updates
  if (currentTime - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
    needsBatteryUpdate = true;
    lastBatteryUpdate = currentTime;
  }
  
  // Timer operation processing
  if (currentScreen == SCREEN_TIMER && statusFlg && 
      currentTime - lastTimerUpdate >= TIMER_INTERVAL) {
    lastTimerUpdate = currentTime;
    
    int orientation = detectOrientation();
    
    // Handle device orientation changes
    if (orientation >= 0 && orientation != currentFace) {
      int previousFace = currentFace;
      currentFace = orientation;
      needsFullRedraw = true;
      needsBatteryUpdate = true;
      
      // Audio feedback for player switching
      if (soundEnabled) {
        isPlayingWarningSound = true;
        playTone(TONE_FREQ_HIGH, TONE_DURATION_SHORT);
        delay(TONE_DURATION_SHORT + 30);
        isPlayingWarningSound = false;
      }
      
      // Reset move timer for new player
      if (currentTimerMode == MOVE_TIMER) {
        switchToNextPlayer();
      }
    }
    
    if (orientation >= 0) {
      // Device is in valid orientation
      if (pauseFlg && !isDisplayOn) {
        turnOnDisplay();
        needsFullRedraw = true;
        needsBatteryUpdate = true;
      }
      
      // Update timer based on current mode
      if (currentTimerMode == GAME_TIMER) {
        updateGameTimer(orientation);
      } else {
        updateMoveTimer(orientation);
      }
      
      // Render updates only when display is active
      if (isDisplayOn && !isPlayingWarningSound) {
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
      
    } else if (accelZ <= ACCEL_THRESHOLD_BACK && !pauseFlg) {
      // Device is face-down (pause mode)
      if (soundEnabled) {
        isPlayingWarningSound = true;
        playTone(TONE_FREQ_MID, TONE_DURATION_WARNING);
        delay(TONE_DURATION_WARNING + 50);
        isPlayingWarningSound = false;
      }
      pauseFlg = true;
      if (isDisplayOn) {
        turnOffDisplay();
      }
    }
  }
  
  // Handle blinking animation for configuration screens
  if (currentScreen != SCREEN_TIMER) {
    if (currentTime - lastBlinkUpdate >= BLINK_INTERVAL) {
      lastBlinkUpdate = currentTime;
      blinkState = !blinkState;
      
      if (isDisplayOn) {
        if (needsFullRedraw) {
          // Full screen redraw
          switch (currentScreen) {
            case SCREEN_SOUND_SELECT:
              drawSoundSelectScreenFull();
              break;
            case SCREEN_MODE_SELECT:
              drawModeSelectScreenFull();
              break;
            default:
              drawSetupModeFull();
              break;
          }
          needsFullRedraw = false;
          needsBatteryUpdate = false;
        } else {
          // Partial update for efficiency
          switch (currentScreen) {
            case SCREEN_SOUND_SELECT:
              updateSoundSelectOnly();
              break;
            case SCREEN_MODE_SELECT:
              updateModeSelectOnly();
              break;
            default:
              updateSetupTimerOnly();
              break;
          }
          if (needsBatteryUpdate) {
            drawBatteryInfo();
            needsBatteryUpdate = false;
          }
        }
      }
    }
  }
  
  // Power saving delay
  delay(10);
}

// =============================================================================
// HARDWARE DETECTION
// =============================================================================

void detectDeviceType() {
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
      // Fallback detection based on touch capability
      deviceType = M5.Touch.isEnabled() ? DEVICE_CORES3 : DEVICE_FIRE;
      break;
  }
}

// =============================================================================
// TIMER LOGIC
// =============================================================================

void updateGameTimer(int faceIndex) {
  if (!isValidPlayerIndex(faceIndex)) return;
  
  int totalTime = timeList[faceIndex];
  
  if (negativeList[faceIndex] != 1) {
    // Normal countdown mode
    if (totalTime > 0) {
      calculateTimeDigits(totalTime, dig01, dig02, dig03, dig04);
      timeList[faceIndex] = totalTime - 1;
      checkTimeWarnings(totalTime, faceIndex);
    } else {
      // Time expired - enter overtime mode
      dig01 = dig02 = dig03 = dig04 = 0;
      negativeList[faceIndex] = 1;
      timeList[faceIndex] = 1;
      
      // Time's up notification
      if (soundEnabled) {
        isPlayingWarningSound = true;
        playTone(TONE_FREQ_LOW, TONE_DURATION_WARNING * 3);
        delay(TONE_DURATION_WARNING * 3 + 100);
        isPlayingWarningSound = false;
      }
    }
  } else {
    // Overtime mode - count up
    calculateTimeDigits(totalTime, dig01, dig02, dig03, dig04);
    timeList[faceIndex] = totalTime + 1;
  }
}

void updateMoveTimer(int faceIndex) {
  if (moveTimeRemaining > 0) {
    calculateTimeDigits(moveTimeRemaining, dig01, dig02, dig03, dig04);
    moveTimeRemaining = moveTimeRemaining - 1;
    checkTimeWarnings(moveTimeRemaining + 1, currentFace);
  } else {
    // Move time expired
    dig01 = dig02 = dig03 = dig04 = 0;
    
    // Move timeout notification
    if (soundEnabled) {
      isPlayingWarningSound = true;
      playTone(TONE_FREQ_LOW, TONE_DURATION_WARNING * 3);
      delay(TONE_DURATION_WARNING * 3 + 100);
      isPlayingWarningSound = false;
    }
  }
}

void switchToNextPlayer() {
  // Reset move timer for new player
  moveTimeRemaining = moveTimerDig01 * SECONDS_PER_TEN_MINUTES + 
                     moveTimerDig02 * SECONDS_PER_MINUTE + 
                     moveTimerDig03 * 10 + 
                     moveTimerDig04;
  resetWarningFlags(currentFace);
}

void checkTimeWarnings(int remainingSeconds, int playerIndex) {
  if (!soundEnabled || !isValidPlayerIndex(playerIndex)) return;
  
  // Play appropriate warning sound based on remaining time
  if (remainingSeconds == WARNING_TIME_10SEC && !warning10secPlayed[playerIndex]) {
    isPlayingWarningSound = true;
    warning10secPlayed[playerIndex] = true;
    playTone(TONE_FREQ_MID, TONE_DURATION_LONG * 2);
    delay(TONE_DURATION_LONG * 2 + 50);
    isPlayingWarningSound = false;
  } else if (remainingSeconds <= WARNING_TIME_5SEC && remainingSeconds >= WARNING_TIME_1SEC) {
    // 5-1 second countdown warnings (unified sound)
    bool* warningFlag = nullptr;
    switch (remainingSeconds) {
      case WARNING_TIME_5SEC: warningFlag = &warning5secPlayed[playerIndex]; break;
      case WARNING_TIME_4SEC: warningFlag = &warning4secPlayed[playerIndex]; break;
      case WARNING_TIME_3SEC: warningFlag = &warning3secPlayed[playerIndex]; break;
      case WARNING_TIME_2SEC: warningFlag = &warning2secPlayed[playerIndex]; break;
      case WARNING_TIME_1SEC: warningFlag = &warning1secPlayed[playerIndex]; break;
    }
    
    if (warningFlag && !*warningFlag) {
      isPlayingWarningSound = true;
      *warningFlag = true;
      playTone(TONE_FREQ_LOW, TONE_DURATION_WARNING * 2);
      delay(TONE_DURATION_WARNING * 2 + 50);
      isPlayingWarningSound = false;
    }
  }
}

void resetWarningFlags(int playerIndex) {
  if (playerIndex == -1) {
    // Reset all players
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (isValidPlayerIndex(i)) {
        warning10secPlayed[i] = false;
        warning5secPlayed[i] = false;
        warning4secPlayed[i] = false;
        warning3secPlayed[i] = false;
        warning2secPlayed[i] = false;
        warning1secPlayed[i] = false;
      }
    }
  } else if (isValidPlayerIndex(playerIndex)) {
    // Reset specific player
    warning10secPlayed[playerIndex] = false;
    warning5secPlayed[playerIndex] = false;
    warning4secPlayed[playerIndex] = false;
    warning3secPlayed[playerIndex] = false;
    warning2secPlayed[playerIndex] = false;
    warning1secPlayed[playerIndex] = false;
  }
  isPlayingWarningSound = false;
}

// =============================================================================
// SCREEN RENDERING - SOUND SELECTION
// =============================================================================

void drawSoundSelectScreenFull() {
  M5.Display.fillScreen(COLOR_BLUE);
  M5.Display.setRotation(1);
  
  // Header elements
  drawBatteryInfo();
  
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(top_center);
  M5.Display.drawString("SOUND SETTING", 160, 30);
  
  // Sound options with selection highlighting
  M5.Display.setTextSize(4);
  M5.Display.setTextDatum(middle_center);
  
  // Sound ON option
  if (soundSelectPosition == 0 && blinkState) {
    M5.Display.setTextColor(COLOR_YELLOW);
  } else if (soundSelectPosition == 0) {
    M5.Display.setTextColor(COLOR_BLUE);
  } else {
    M5.Display.setTextColor(COLOR_WHITE);
  }
  M5.Display.drawString("Sound ON", 160, 90);
  
  // Sound OFF option
  if (soundSelectPosition == 1 && blinkState) {
    M5.Display.setTextColor(COLOR_YELLOW);
  } else if (soundSelectPosition == 1) {
    M5.Display.setTextColor(COLOR_BLUE);
  } else {
    M5.Display.setTextColor(COLOR_WHITE);
  }
  M5.Display.drawString("Sound OFF", 160, 130);
  
  // Device-specific UI elements
  if (deviceType == DEVICE_CORES3) {
    drawTouchButtons();
  } else {
    drawFireButtonHelp();
  }
}

void updateSoundSelectOnly() {
  // Efficient partial update of sound options
  M5.Display.fillRect(50, 70, 220, 80, COLOR_BLUE);
  
  M5.Display.setTextSize(4);
  M5.Display.setTextDatum(middle_center);
  
  // Sound ON option
  if (soundSelectPosition == 0 && blinkState) {
    M5.Display.setTextColor(COLOR_YELLOW);
  } else if (soundSelectPosition == 0) {
    M5.Display.setTextColor(COLOR_BLUE);
  } else {
    M5.Display.setTextColor(COLOR_WHITE);
  }
  M5.Display.drawString("Sound ON", 160, 90);
  
  // Sound OFF option
  if (soundSelectPosition == 1 && blinkState) {
    M5.Display.setTextColor(COLOR_YELLOW);
  } else if (soundSelectPosition == 1) {
    M5.Display.setTextColor(COLOR_BLUE);
  } else {
    M5.Display.setTextColor(COLOR_WHITE);
  }
  M5.Display.drawString("Sound OFF", 160, 130);
}

// =============================================================================
// SCREEN RENDERING - MODE SELECTION
// =============================================================================

void drawModeSelectScreenFull() {
  M5.Display.fillScreen(COLOR_BLUE);
  M5.Display.setRotation(1);

  drawBatteryInfo();

  M5.Display.setTextSize(3);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(top_center);
  M5.Display.drawString("SELECT MODE", getScreenCenterX(), getTitleY());

  M5.Display.setTextSize(4);
  M5.Display.setTextDatum(middle_center);

  // Game Timer option
  M5.Display.setTextColor((modeSelectPosition == 0 && blinkState) ? COLOR_YELLOW :
                          (modeSelectPosition == 0) ? COLOR_BLUE : COLOR_WHITE);
  M5.Display.drawString("Game Timer", getScreenCenterX(), getOptionY(0));

  // Move Timer option
  M5.Display.setTextColor((modeSelectPosition == 1 && blinkState) ? COLOR_YELLOW :
                          (modeSelectPosition == 1) ? COLOR_BLUE : COLOR_WHITE);
  M5.Display.drawString("Move Timer", getScreenCenterX(), getOptionY(1));

  if (deviceType == DEVICE_CORES3) {
    drawTouchButtons();
  } else {
    drawFireButtonHelp();
  }
}

void updateModeSelectOnly() {
  // Efficient partial update of mode options
  M5.Display.fillRect(50, 70, 220, 80, COLOR_BLUE);
  
  M5.Display.setTextSize(4);
  M5.Display.setTextDatum(middle_center);
  
  // Game Timer option
  if (modeSelectPosition == 0 && blinkState) {
    M5.Display.setTextColor(COLOR_YELLOW);
  } else if (modeSelectPosition == 0) {
    M5.Display.setTextColor(COLOR_BLUE);
  } else {
    M5.Display.setTextColor(COLOR_WHITE);
  }
  M5.Display.drawString("Game Timer", 160, 90);
  
  // Move Timer option
  if (modeSelectPosition == 1 && blinkState) {
    M5.Display.setTextColor(COLOR_YELLOW);
  } else if (modeSelectPosition == 1) {
    M5.Display.setTextColor(COLOR_BLUE);
  } else {
    M5.Display.setTextColor(COLOR_WHITE);
  }
  M5.Display.drawString("Move Timer", 160, 130);
}

// =============================================================================
// SCREEN RENDERING - SETUP/CONFIGURATION
// =============================================================================

void drawSetupModeFull() {
  M5.Display.fillScreen(COLOR_BLUE);
  M5.Display.setRotation(1);

  drawBatteryInfo();

  M5.Display.setTextSize(3);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(top_center);

  formatModeTitle(currentTimerMode, titleBuffer);
  M5.Display.drawString(titleBuffer, getScreenCenterX(), getTitleY());

  M5.Display.setTextSize(6);
  formatTime(dig01, dig02, dig03, dig04, timeBuffer);

  if (!blinkState) {
    switch (updatePosition) {
      case 0: timeBuffer[0] = ' '; break;
      case 1: timeBuffer[1] = ' '; break;
      case 2: timeBuffer[3] = ' '; break;
      case 3: timeBuffer[4] = ' '; break;
    }
  }

  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(timeBuffer, getScreenCenterX(), getTimerY());

  if (deviceType == DEVICE_CORES3) {
    drawTouchButtons();
  } else {
    drawFireButtonHelp();
  }
}

void updateSetupTimerOnly() {
  // Efficient partial update of time display
  DisplayArea timerArea = getTimerDisplayArea();
  M5.Display.fillRect(timerArea.x, timerArea.y, timerArea.width, timerArea.height, COLOR_BLUE);
  
  M5.Display.setTextSize(6);
  formatTime(dig01, dig02, dig03, dig04, timeBuffer);
  
  // Apply blinking effect to current digit
  if (!blinkState) {
    switch (updatePosition) {
      case 0: timeBuffer[0] = ' '; break;  // Minutes tens
      case 1: timeBuffer[1] = ' '; break;  // Minutes ones
      case 2: timeBuffer[3] = ' '; break;  // Seconds tens
      case 3: timeBuffer[4] = ' '; break;  // Seconds ones
    }
  }
  
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(timeBuffer, getScreenCenterX(), getTimerY());
}

// =============================================================================
// SCREEN RENDERING - TIMER OPERATION
// =============================================================================

void drawTimerDisplayFull() {
  M5.Display.fillScreen(COLOR_BLUE);
  
  // Set screen rotation based on current player orientation
  switch (currentFace) {
    case 0: M5.Display.setRotation(1); break;  // P1 - front
    case 1: M5.Display.setRotation(2); break;  // P2 - right
    case 2: M5.Display.setRotation(3); break;  // P3 - back
    case 3: M5.Display.setRotation(0); break;  // P4 - left
    case 4: M5.Display.setRotation(1); break;  // P5 - front
  }
  
  // Calculate screen dimensions after rotation
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int centerX = screenW / 2;
  int centerY = screenH / 2;
  
  // Header elements
  drawBatteryInfo();
  
  // Player and mode information
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(top_center);
  
  sprintf(titleBuffer, "P%d %s", currentFace + 1, 
          (currentTimerMode == GAME_TIMER) ? "GAME" : "MOVE");
  M5.Display.drawString(titleBuffer, centerX, 20);
  
  // Timer display with appropriate color coding
  uint32_t timerColor = COLOR_WHITE;
  if (currentTimerMode == GAME_TIMER && isValidPlayerIndex(currentFace) && 
      negativeList[currentFace] == 1) {
    timerColor = COLOR_RED;  // Overtime in game mode
  } else if (currentTimerMode == MOVE_TIMER && moveTimeRemaining == 0) {
    timerColor = COLOR_RED;  // Move time expired
  }
  
  M5.Display.setTextSize(6);
  M5.Display.setTextColor(timerColor);
  M5.Display.setTextDatum(middle_center);
  formatTime(dig01, dig02, dig03, dig04, timeBuffer);
  M5.Display.drawString(timeBuffer, centerX, centerY);
  
  // Status indicator
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(bottom_center);
  const char* status = statusFlg ? "RUNNING" : "STOPPED";
  M5.Display.drawString(status, centerX, screenH - 60);
  
  // Device-specific UI elements
  if (deviceType == DEVICE_CORES3) {
    drawTouchButtons();
  } else {
    drawFireButtonHelp();
  }
}

void updateTimerOnly() {
  // Efficient partial update of timer display
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int centerX = screenW / 2;
  int centerY = screenH / 2;
  
  DisplayArea timerArea = getTimerDisplayArea();
  M5.Display.fillRect(centerX - timerArea.width/2, centerY - timerArea.height/2, 
                      timerArea.width, timerArea.height, COLOR_BLUE);
  
  // Timer color based on current state
  uint32_t timerColor = COLOR_WHITE;
  if (currentTimerMode == GAME_TIMER && isValidPlayerIndex(currentFace) && 
      negativeList[currentFace] == 1) {
    timerColor = COLOR_RED;
  } else if (currentTimerMode == MOVE_TIMER && moveTimeRemaining == 0) {
    timerColor = COLOR_RED;
  }
  
  M5.Display.setTextSize(6);
  M5.Display.setTextColor(timerColor);
  M5.Display.setTextDatum(middle_center);
  formatTime(dig01, dig02, dig03, dig04, timeBuffer);
  M5.Display.drawString(timeBuffer, centerX, centerY);
}

// Compatibility functions
void drawTimerDisplay() {
  drawTimerDisplayFull();
}

void drawSetupMode() {
  drawSetupModeFull();
}

// =============================================================================
// BATTERY AND SYSTEM INFO DISPLAY
// =============================================================================

void drawBatteryInfo() {
  int screenW = M5.Display.width();
  
  bool isCharging = M5.Power.isCharging();
  int batteryLevel = M5.Power.getBatteryLevel();
  
  // Clear previous display area
  DisplayArea batteryArea = getBatteryDisplayArea();
  M5.Display.fillRect(batteryArea.x, batteryArea.y, batteryArea.width, batteryArea.height, COLOR_BLUE);
  
  // Color coding based on battery level and charging state
  uint32_t batteryColor = COLOR_WHITE;
  if (isCharging) {
    batteryColor = COLOR_GREEN;
  } else if (batteryLevel <= 20) {
    batteryColor = COLOR_RED;
  } else if (batteryLevel <= 50) {
    batteryColor = COLOR_YELLOW;
  } else {
    batteryColor = COLOR_GREEN;
  }
  
  // Display formatted battery information
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(batteryColor);
  M5.Display.setTextDatum(top_right);
  
  formatBatteryText(batteryLevel, isCharging, batteryBuffer);
  M5.Display.drawString(batteryBuffer, screenW - 5, 5);
}

// =============================================================================
// INPUT HANDLING
// =============================================================================

void handleButtons() {
  // Check for any button activity to wake display
  bool anyButtonPressed = false;
  
  // Physical button detection
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    anyButtonPressed = true;
  }
  
  // Touch detection for CoreS3
  if (deviceType == DEVICE_CORES3 && M5.Touch.getCount() > 0) {
    auto touch = M5.Touch.getDetail(0);
    if (touch.wasPressed()) {
      anyButtonPressed = true;
    }
  }
  
  // Wake display if sleeping
  if (!isDisplayOn && anyButtonPressed) {
    turnOnDisplay();
    needsFullRedraw = true;
    needsBatteryUpdate = true;
    return;  // Process input in next loop cycle
  }
  
  // Process device-specific input
  handlePhysicalButtons();
  if (deviceType == DEVICE_CORES3) {
    handleTouchButtons();
  }
}

void handlePhysicalButtons() {
  bool leftPressed = M5.BtnA.wasPressed();
  bool centerPressed = M5.BtnB.wasPressed();
  bool rightPressed = M5.BtnC.wasPressed();
  
  // Long press detection for right button (back to mode select)
  if (M5.BtnC.isPressed() && btnCPressTime == 0) {
    btnCPressTime = millis();
    btnCLongPressed = false;
  } else if (M5.BtnC.isPressed() && !btnCLongPressed) {
    if (millis() - btnCPressTime >= LONG_PRESS_DURATION) {
      btnCLongPressed = true;
      if (currentScreen == SCREEN_SETUP) {
        resetToModeSelect();
        btnCPressTime = 0;
        return;
      }
    }
  } else if (!M5.BtnC.isPressed()) {
    btnCPressTime = 0;
    btnCLongPressed = false;
  }
  
  // Process button actions based on current screen
  switch (currentScreen) {
    case SCREEN_SOUND_SELECT:
      if (leftPressed || rightPressed) {
        soundSelectPosition = (soundSelectPosition == 0) ? 1 : 0;
      }
      if (centerPressed) {
        soundEnabled = (soundSelectPosition == 0);
        currentScreen = SCREEN_MODE_SELECT;
        modeSelectPosition = 0;
        needsFullRedraw = true;
        needsBatteryUpdate = true;
      }
      break;
      
    case SCREEN_MODE_SELECT:
      if (leftPressed || rightPressed) {
        modeSelectPosition = (modeSelectPosition == 0) ? 1 : 0;
      }
      if (centerPressed) {
        currentTimerMode = (modeSelectPosition == 0) ? GAME_TIMER : MOVE_TIMER;
        currentScreen = SCREEN_SETUP;
        updatePosition = 0;
        needsFullRedraw = true;
        needsBatteryUpdate = true;
      }
      break;
      
    case SCREEN_SETUP:
      if (leftPressed) {
        incrementDigit();
      }
      if (centerPressed) {
        // Start timer operation
        initializeTimers();
        resetWarningFlags(-1);
        statusFlg = true;
        startFlg = true;
        warningPlayed = false;
        lastTimerUpdate = millis();
        currentScreen = SCREEN_TIMER;
        needsFullRedraw = true;
        needsBatteryUpdate = true;
        
        int orientation = detectOrientation();
        currentFace = (orientation >= 0) ? orientation : 4;
      }
      if (rightPressed && !btnCLongPressed) {
        nextDigitPosition();
      }
      break;
      
    case SCREEN_TIMER:
      // Modified: Disable A button completely during timer operation
      if (leftPressed && !statusFlg) {
        resetToSetup();
      }
      // A button is completely ignored when statusFlg is true
      
      if (centerPressed) {
        if (!statusFlg) {
          // Resume timer
          resetWarningFlags(-1);
          statusFlg = true;
          lastTimerUpdate = millis();
          needsFullRedraw = true;
          needsBatteryUpdate = true;
          
          int orientation = detectOrientation();
          currentFace = (orientation >= 0) ? orientation : 4;
        } else {
          // Pause timer
          statusFlg = false;
          needsFullRedraw = true;
          needsBatteryUpdate = true;
          if (isDisplayOn) {
            drawTimerDisplayFull();
          }
        }
      }
      break;
  }
}

void handleTouchButtons() {
  bool leftPressed = false;
  bool centerPressed = false;
  bool rightPressed = false;
  bool rightLongPressed = false;
  
  // Touch event processing
  if (M5.Touch.getCount() > 0) {
    auto touch = M5.Touch.getDetail(0);
    int touchX = touch.x;
    int touchY = touch.y;
    int screenW = M5.Display.width();
    int screenH = M5.Display.height();
    int buttonHeight = getButtonHeight();
    int buttonY = screenH - buttonHeight;
    int buttonWidth = screenW / 3;
    
    // Right button long press detection
    bool rightButtonTouched = (touchY >= buttonY && touchY <= screenH && 
                              touchX >= buttonWidth * 2 && touchX < screenW);
    
    if (rightButtonTouched && touch.isPressed()) {
      if (!touchRightCurrentlyPressed) {
        touchRightPressTime = millis();
        touchRightCurrentlyPressed = true;
        touchRightLongPressed = false;
      } else if (!touchRightLongPressed && currentScreen == SCREEN_SETUP) {
        if (millis() - touchRightPressTime >= LONG_PRESS_DURATION) {
          touchRightLongPressed = true;
          rightLongPressed = true;
        }
      }
    } else if (!rightButtonTouched || !touch.isPressed()) {
      touchRightCurrentlyPressed = false;
      touchRightLongPressed = false;
      touchRightPressTime = 0;
    }
    
    // Standard button press detection
    if (touch.wasPressed() && !rightLongPressed) {
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
  } else {
    // Reset touch state when no contact
    touchRightCurrentlyPressed = false;
    touchRightLongPressed = false;
    touchRightPressTime = 0;
  }
  
  // Process touch actions (same logic as physical buttons)
  switch (currentScreen) {
    case SCREEN_SOUND_SELECT:
      if (leftPressed || rightPressed) {
        soundSelectPosition = (soundSelectPosition == 0) ? 1 : 0;
      }
      if (centerPressed) {
        soundEnabled = (soundSelectPosition == 0);
        currentScreen = SCREEN_MODE_SELECT;
        modeSelectPosition = 0;
        needsFullRedraw = true;
        needsBatteryUpdate = true;
      }
      break;
      
    case SCREEN_MODE_SELECT:
      if (leftPressed || rightPressed) {
        modeSelectPosition = (modeSelectPosition == 0) ? 1 : 0;
      }
      if (centerPressed) {
        currentTimerMode = (modeSelectPosition == 0) ? GAME_TIMER : MOVE_TIMER;
        currentScreen = SCREEN_SETUP;
        updatePosition = 0;
        needsFullRedraw = true;
        needsBatteryUpdate = true;
      }
      break;
      
    case SCREEN_SETUP:
      if (rightLongPressed) {
        resetToModeSelect();
      } else if (leftPressed) {
        incrementDigit();
      } else if (centerPressed) {
        // Start timer operation
        initializeTimers();
        resetWarningFlags(-1);
        statusFlg = true;
        startFlg = true;
        warningPlayed = false;
        lastTimerUpdate = millis();
        currentScreen = SCREEN_TIMER;
        needsFullRedraw = true;
        needsBatteryUpdate = true;
        
        int orientation = detectOrientation();
        currentFace = (orientation >= 0) ? orientation : 4;
      } else if (rightPressed) {
        nextDigitPosition();
      }
      break;
      
    case SCREEN_TIMER:
      // Modified: Disable left touch completely during timer operation
      if (leftPressed && !statusFlg) {
        resetToSetup();
      }
      // Left touch is completely ignored when statusFlg is true
      
      if (centerPressed) {
        if (!statusFlg) {
          // Resume timer
          resetWarningFlags(-1);
          statusFlg = true;
          lastTimerUpdate = millis();
          needsFullRedraw = true;
          needsBatteryUpdate = true;
          
          int orientation = detectOrientation();
          currentFace = (orientation >= 0) ? orientation : 4;
        } else {
          // Pause timer
          statusFlg = false;
          needsFullRedraw = true;
          needsBatteryUpdate = true;
          if (isDisplayOn) {
            drawTimerDisplayFull();
          }
        }
      }
      break;
  }
}

// =============================================================================
// IMU AND ORIENTATION DETECTION
// =============================================================================

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
  // Map accelerometer readings to player faces
  if (accelY >= ACCEL_THRESHOLD) return 0;  // P1 face (Y+ forward)
  if (accelX >= ACCEL_THRESHOLD) return 1;  // P2 face (X+ right)
  if (accelY <= -ACCEL_THRESHOLD) return 2; // P3 face (Y- back)
  if (accelX <= -ACCEL_THRESHOLD) return 3; // P4 face (X- left)
  if (accelZ >= ACCEL_THRESHOLD) return 4;  // P5 face (Z+ up)
  return -1; // No clear orientation detected
}

// =============================================================================
// AUDIO OUTPUT
// =============================================================================

void playTone(int frequency, int duration) {
  M5.Speaker.tone(frequency, duration);
}

// =============================================================================
// NAVIGATION AND STATE MANAGEMENT
// =============================================================================

void resetToModeSelect() {
  currentScreen = SCREEN_MODE_SELECT;
  modeSelectPosition = 0;
  statusFlg = false;
  startFlg = false;
  resetWarningFlags(-1);
  needsFullRedraw = true;
  needsBatteryUpdate = true;
  
  if (!isDisplayOn) {
    turnOnDisplay();
  }
}

void resetToSetup() {
  currentScreen = SCREEN_SETUP;
  updatePosition = 0;
  statusFlg = false;
  startFlg = false;
  resetWarningFlags(-1);
  needsFullRedraw = true;
  needsBatteryUpdate = true;
  
  if (!isDisplayOn) {
    turnOnDisplay();
  }
  
  // Restore original setup values
  dig01 = initDig01;
  dig02 = initDig02;
  dig03 = initDig03;
  dig04 = initDig04;
}

void resetCurrentTimer() {
  statusFlg = false;
  startFlg = false;
  currentFace = 4;
  warningPlayed = false;
  resetWarningFlags(-1);
  needsFullRedraw = true;
  needsBatteryUpdate = true;
  
  if (!isDisplayOn) {
    turnOnDisplay();
  }
  
  initializeTimers();
}

void initializeTimers() {
  int totalTime = dig01 * SECONDS_PER_TEN_MINUTES + 
                 dig02 * SECONDS_PER_MINUTE + 
                 dig03 * 10 + 
                 dig04;
  
  if (currentTimerMode == GAME_TIMER) {
    // Initialize all players with same time
    for (int i = 0; i < MAX_PLAYERS; i++) {
      timeList[i] = totalTime;
      negativeList[i] = 0;
    }
  } else {
    // Initialize move timer
    moveTimerDig01 = dig01;
    moveTimerDig02 = dig02;
    moveTimerDig03 = dig03;
    moveTimerDig04 = dig04;
    moveTimeRemaining = totalTime;
  }
}

void incrementDigit() {
  // Increment current digit with wrap-around
  switch (updatePosition) {
    case 0:
      dig01 = (dig01 < MAX_DIGIT_TENS_MINUTES) ? dig01 + 1 : 0;
      break;
    case 1:
      dig02 = (dig02 < MAX_DIGIT_ONES) ? dig02 + 1 : 0;
      break;
    case 2:
      dig03 = (dig03 < MAX_DIGIT_TENS_SECONDS) ? dig03 + 1 : 0;
      break;
    case 3:
      dig04 = (dig04 < MAX_DIGIT_ONES) ? dig04 + 1 : 0;
      break;
  }
  
  // Save current settings
  initDig01 = dig01;
  initDig02 = dig02;
  initDig03 = dig03;
  initDig04 = dig04;
}

void nextDigitPosition() {
  // Move to next digit position (circular)
  updatePosition = (updatePosition + 1) % 4;
}

// =============================================================================
// DISPLAY MANAGEMENT
// =============================================================================

void turnOffDisplay() {
  if (isDisplayOn) {
    M5.Display.setBrightness(0);
    isDisplayOn = false;
  }
}

void turnOnDisplay() {
  if (!isDisplayOn) {
    M5.Display.setBrightness(128);
    isDisplayOn = true;
  }
}

void drawTouchButtons() {
  // TouchScreen UI for CoreS3
  if (deviceType != DEVICE_CORES3) return;
  
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int buttonHeight = getButtonHeight();
  int buttonY = screenH - buttonHeight;
  int buttonWidth = screenW / 3;
  
  // Clear and draw button area
  M5.Display.fillRect(0, buttonY, screenW, buttonHeight, COLOR_BLUE);
  M5.Display.drawLine(buttonWidth, buttonY, buttonWidth, screenH, COLOR_WHITE);
  M5.Display.drawLine(buttonWidth * 2, buttonY, buttonWidth * 2, screenH, COLOR_WHITE);
  M5.Display.drawLine(0, buttonY, screenW, buttonY, COLOR_WHITE);
  
  // Setup screen specific help text
  if (currentScreen == SCREEN_SETUP) {
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(COLOR_WHITE);
    M5.Display.setTextDatum(bottom_center);
    M5.Display.drawString("(Hold NEXT: Back to Mode)", screenW / 2, buttonY - 5);
  }
  
  // Button labels based on current screen
  M5.Display.setTextSize(BUTTON_TEXT_SIZE);
  M5.Display.setTextDatum(middle_center);
  
  switch (currentScreen) {
    case SCREEN_SOUND_SELECT:
    case SCREEN_MODE_SELECT:
      M5.Display.setTextColor(COLOR_WHITE);
      M5.Display.drawString("TOGGLE", buttonWidth / 2, buttonY + buttonHeight / 2);
      M5.Display.drawString("SELECT", buttonWidth + buttonWidth / 2, buttonY + buttonHeight / 2);
      M5.Display.drawString("", buttonWidth * 2 + buttonWidth / 2, buttonY + buttonHeight / 2);
      break;
      
    case SCREEN_SETUP:
      M5.Display.setTextColor(COLOR_WHITE);
      M5.Display.drawString("+1", buttonWidth / 2, buttonY + buttonHeight / 2);
      M5.Display.drawString("START", buttonWidth + buttonWidth / 2, buttonY + buttonHeight / 2);
      M5.Display.drawString("NEXT", buttonWidth * 2 + buttonWidth / 2, buttonY + buttonHeight / 2);
      break;
      
    case SCREEN_TIMER:
      if (statusFlg) {
        // Timer running - left button disabled
        M5.Display.setTextColor(COLOR_BLUE);
        M5.Display.drawString("-", buttonWidth / 2, buttonY + buttonHeight / 2);
        M5.Display.setTextColor(COLOR_WHITE);
        M5.Display.drawString("STOP", buttonWidth + buttonWidth / 2, buttonY + buttonHeight / 2);
      } else {
        // Timer stopped - full actions
        M5.Display.setTextColor(COLOR_WHITE);
        M5.Display.drawString("BACK", buttonWidth / 2, buttonY + buttonHeight / 2);
        M5.Display.drawString("START", buttonWidth + buttonWidth / 2, buttonY + buttonHeight / 2);
      }
      M5.Display.drawString("", buttonWidth * 2 + buttonWidth / 2, buttonY + buttonHeight / 2);
      break;
  }
}

void drawFireButtonHelp() {
  // Physical button help for Fire
  if (deviceType != DEVICE_FIRE) return;
  
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int centerX = screenW / 2;
  
  M5.Display.setTextSize(2);
  M5.Display.setTextDatum(bottom_center);
  
  switch (currentScreen) {
    case SCREEN_SOUND_SELECT:
    case SCREEN_MODE_SELECT:
      M5.Display.setTextColor(COLOR_WHITE);
      M5.Display.drawString("A:Toggle  B:Select", centerX, screenH - 20);
      break;
      
    case SCREEN_SETUP:
      M5.Display.setTextColor(COLOR_WHITE);
      M5.Display.drawString("A:+1  B:Start  C:Next", centerX, screenH - 35);
      M5.Display.setTextSize(1);
      M5.Display.drawString("(Hold C: Back to Mode)", centerX, screenH - 15);
      break;
      
    case SCREEN_TIMER:
      if (statusFlg) {
        // Timer running - A button disabled
        M5.Display.setTextColor(COLOR_BLUE);
        M5.Display.drawString("A:-", centerX, screenH - 35);
        M5.Display.setTextColor(COLOR_WHITE);
        M5.Display.drawString("B:Stop", centerX, screenH - 15);
      } else {
        // Timer stopped
        M5.Display.setTextColor(COLOR_WHITE);
        M5.Display.drawString("A:Back  B:Start", centerX, screenH - 20);
      }
      break;
  }
}

int getButtonHeight() {
  // Calculate touch button height for CoreS3
  if (deviceType != DEVICE_CORES3) return 0;
  return (BUTTON_TEXT_SIZE * 8) + (BUTTON_MARGIN * 2);
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

bool isValidPlayerIndex(int playerIndex) {
  return (playerIndex >= 0 && playerIndex < MAX_PLAYERS);
}

void calculateTimeDigits(int totalSeconds, int& dig1, int& dig2, int& dig3, int& dig4) {
  // Convert total seconds to MM:SS format digits
  dig1 = totalSeconds / SECONDS_PER_TEN_MINUTES;
  int remainder = totalSeconds % SECONDS_PER_TEN_MINUTES;
  dig2 = remainder / SECONDS_PER_MINUTE;
  int secondsRemainder = remainder % SECONDS_PER_MINUTE;
  dig3 = secondsRemainder / 10;
  dig4 = secondsRemainder % 10;
}

DisplayArea getTimerDisplayArea() {
  // Standard timer display area for clearing background
  return {160 - 100, 120 - 30, 200, 60};
}

DisplayArea getBatteryDisplayArea() {
  int screenW = M5.Display.width();
  return {screenW - 50, 5, 45, 20};
}

DisplayArea getButtonArea() {
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int buttonHeight = getButtonHeight();
  return {0, screenH - buttonHeight, screenW, buttonHeight};
}

void formatTime(int dig1, int dig2, int dig3, int dig4, char* buffer) {
  // Format time as MM:SS string
  sprintf(buffer, "%d%d:%d%d", dig1, dig2, dig3, dig4);
}

void formatBatteryText(int batteryLevel, bool isCharging, char* buffer) {
  // Format battery percentage with charging indicator
  if (isCharging) {
    sprintf(buffer, "+%d%%", batteryLevel);
  } else {
    sprintf(buffer, "%d%%", batteryLevel);
  }
}

void formatModeTitle(TimerMode mode, char* buffer) {
  // Format timer mode title
  if (mode == GAME_TIMER) {
    strcpy(buffer, "GAME TIMER");
  } else {
    strcpy(buffer, "MOVE TIMER");
  }
}

uint32_t getSelectColor(bool selected, bool blink) {
  if (selected && blink) return COLOR_YELLOW;
  if (selected) return COLOR_BLUE;
  return COLOR_WHITE;
}

void drawSelectMenu(const char* title, const char* options[], int optionCount, int selectedIdx) {
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(top_center);
  M5.Display.drawString(title, getScreenCenterX(), getTitleY());

  M5.Display.setTextSize(4);
  M5.Display.setTextDatum(middle_center);
  for (int i = 0; i < optionCount; ++i) {
    M5.Display.setTextColor(getSelectColor(selectedIdx == i, blinkState));
    M5.Display.drawString(options[i], getScreenCenterX(), getOptionY(i));
  }
}

uint32_t getBatteryColor(int batteryLevel, bool isCharging) {
  if (isCharging) return COLOR_GREEN;
  if (batteryLevel <= 20) return COLOR_RED;
  if (batteryLevel <= 50) return COLOR_YELLOW;
  return COLOR_GREEN;
}
