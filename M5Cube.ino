#include <M5Unified.h>

// 定数定義
const float ACCEL_THRESHOLD = 0.7f;
const float ACCEL_THRESHOLD_BACK = -0.7f;
const int TONE_FREQ_HIGH = 2800;
const int TONE_FREQ_LOW = 1000;
const int TONE_DURATION_LONG = 100;
const int TONE_DURATION_WARNING = 100;
const int TIMER_INTERVAL = 1000;
const int BLINK_INTERVAL = 400;
const int IMU_UPDATE_INTERVAL = 20; // IMU更新間隔（ms）
const int BATTERY_UPDATE_INTERVAL = 5000; // バッテリー表示更新間隔（ms）
const int PAUSE_IMU_INTERVAL = 100; // ポーズ時のIMU更新間隔（ms）

// 色定義
const uint32_t COLOR_WHITE = 0xFFFFFF;
const uint32_t COLOR_RED = 0xFF0000;
const uint32_t COLOR_BLUE = 0x000066;
const uint32_t COLOR_GREEN = 0x00FF00;
const uint32_t COLOR_YELLOW = 0xFFFF00;

// グローバル変数
int timeList[5] = {0, 0, 0, 0, 0};
int negativeList[5] = {0, 0, 0, 0, 0};
int currentFace = 4; // 現在表示中の面（0-4）
bool statusFlg = false;
bool startFlg = false;
bool pauseFlg = false;
int updatePosition = 0;
int dig01 = 0, dig02 = 0, dig03 = 0, dig04 = 0;
int initDig01 = 0, initDig02 = 0, initDig03 = 0, initDig04 = 0;

unsigned long lastTimerUpdate = 0;
unsigned long lastBlinkUpdate = 0;
unsigned long lastIMUUpdate = 0; // IMU更新管理用
unsigned long lastBatteryUpdate = 0; // バッテリー表示更新管理用
bool blinkState = true;
bool warningPlayed = false; // 警告音再生フラグ
bool needsFullRedraw = true; // 全体再描画フラグ
bool needsBatteryUpdate = true; // バッテリー表示更新フラグ
bool isDisplayOn = true; // 画面ON/OFFフラグ

// IMU データ
float accelX, accelY, accelZ;

// 関数プロトタイプ
void updateTimer(int faceIndex);
void drawTimerDisplay();
void drawTimerDisplayFull(); // 全体描画用
void updateTimerOnly(); // タイマー部分のみ更新用
void drawSetupMode();
void drawSetupModeFull(); // セットアップ画面全体描画用
void updateSetupTimerOnly(); // セットアップ画面タイマー部分のみ更新用
void handleButtons();
void updateIMU();
int detectOrientation();
void playTone(int frequency, int duration);
void resetAllTimers();
void incrementDigit();
void nextDigitPosition();
String formatTime(int dig1, int dig2, int dig3, int dig4);
void drawBatteryInfo(); // バッテリー表示用関数を追加
void turnOffDisplay(); // 画面消灯用関数
void turnOnDisplay(); // 画面復帰用関数

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  // 省電力設定
  M5.Display.setBrightness(128); // 画面輝度を50%に設定（0-255）
  setCpuFrequencyMhz(80); // CPU周波数を80MHzに設定（デフォルト240MHz）
  
  M5.Display.begin();
  M5.Display.fillScreen(COLOR_BLUE);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(middle_center);
  
  M5.Imu.begin();
  M5.Speaker.begin();
  
  // 初期表示
  currentFace = 4; // P5面（設定面）
  needsFullRedraw = true; // 初期化時は全体描画
  needsBatteryUpdate = true; // 初期化時はバッテリー表示更新
  isDisplayOn = true; // 初期状態では画面ON
  lastBatteryUpdate = millis();
  drawSetupModeFull();
}

void loop() {
  M5.update();
  
  // IMU更新頻度の制御（省電力化）
  unsigned long currentTime = millis();
  bool shouldUpdateIMU = false;
  
  if (pauseFlg) {
    // ポーズ中は低頻度でIMU更新
    if (currentTime - lastIMUUpdate >= PAUSE_IMU_INTERVAL) {
      shouldUpdateIMU = true;
      lastIMUUpdate = currentTime;
    }
  } else {
    // 通常時は高頻度でIMU更新
    if (currentTime - lastIMUUpdate >= IMU_UPDATE_INTERVAL) {
      shouldUpdateIMU = true;
      lastIMUUpdate = currentTime;
    }
  }
  
  if (shouldUpdateIMU) {
    updateIMU();
  }
  
  handleButtons();
  
  // バッテリー表示の更新頻度制御
  if (currentTime - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
    needsBatteryUpdate = true;
    lastBatteryUpdate = currentTime;
  }
  
  // タイマー動作中の処理
  if (statusFlg && currentTime - lastTimerUpdate >= TIMER_INTERVAL) {
    lastTimerUpdate = currentTime;
    
    int orientation = detectOrientation();
    if (orientation >= 0 && orientation != currentFace) {
      // 面切り替え
      currentFace = orientation;
      needsFullRedraw = true; // 面切り替え時は全体再描画
      needsBatteryUpdate = true; // 面切り替え時はバッテリー更新
      playTone(TONE_FREQ_HIGH, TONE_DURATION_LONG);
    }
    
    if (orientation >= 0) {
      // 伏せていた状態から復帰した場合の処理
      if (pauseFlg && !isDisplayOn) {
        turnOnDisplay();
        needsFullRedraw = true; // 画面復帰時は全体再描画
        needsBatteryUpdate = true;
      }
      
      updateTimer(orientation);
      
      // 画面がONの場合のみ描画処理を実行
      if (isDisplayOn) {
        if (needsFullRedraw) {
          drawTimerDisplayFull();
          needsFullRedraw = false;
          needsBatteryUpdate = false; // 全体描画時はバッテリー更新済み
        } else {
          updateTimerOnly(); // タイマー部分のみ更新
          // バッテリー更新が必要な場合のみ更新
          if (needsBatteryUpdate) {
            drawBatteryInfo();
            needsBatteryUpdate = false;
          }
        }
      }
      pauseFlg = false;
    } else if (accelZ <= ACCEL_THRESHOLD_BACK && pauseFlg == false) {
      // 伏せた状態になった時の処理
      playTone(TONE_FREQ_LOW, TONE_DURATION_WARNING);
      pauseFlg = true;
      // 画面を消灯（省電力化）
      if (isDisplayOn) {
        turnOffDisplay();
      }
    }
  }
  
  // 設定モード時の点滅処理
  if (!startFlg) {
    if (currentTime - lastBlinkUpdate >= BLINK_INTERVAL) {
      lastBlinkUpdate = currentTime;
      blinkState = !blinkState;
      
      // 画面がONの場合のみ描画処理を実行
      if (isDisplayOn) {
        if (needsFullRedraw) {
          drawSetupModeFull();
          needsFullRedraw = false;
          needsBatteryUpdate = false; // 全体描画時はバッテリー更新済み
        } else {
          updateSetupTimerOnly(); // タイマー部分のみ更新
          // バッテリー更新が必要な場合のみ更新
          if (needsBatteryUpdate) {
            drawBatteryInfo();
            needsBatteryUpdate = false;
          }
        }
      }
    }
  }
  
  delay(10); // delayを2msから10msに増加（省電力化）
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
      // totalTime == 0の時は白文字で00:00を表示
      dig01 = dig02 = dig03 = dig04 = 0;
      negativeList[faceIndex] = 1;  // 次回から超過時間モードに入る
      timeList[faceIndex] = 1;      // 次回は1秒超過から開始
    }
  } else {
    // 超過時間の処理（1秒、2秒、3秒...）
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
  
  // 面に応じて画面回転を設定
  switch (currentFace) {
    case 0: // P1面 - 0度（正面）
      M5.Display.setRotation(1);
      break;
    case 1: // P2面 - 90度（右回転）
      M5.Display.setRotation(2);
      break;
    case 2: // P3面 - 180度（逆さま）
      M5.Display.setRotation(3);
      break;
    case 3: // P4面 - 270度（左回転）
      M5.Display.setRotation(0);
      break;
    case 4: // P5面 - 0度（正面）
      M5.Display.setRotation(1);
      break;
  }
  
  // 回転後の画面サイズを取得
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int centerX = screenW / 2;
  int centerY = screenH / 2;
  
  // バッテリー情報表示（右上）
  drawBatteryInfo();
  
  // 現在の面表示
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(top_center);
  M5.Display.drawString("Player:" + String(currentFace + 1), centerX, 20);
  
  // タイマー表示の色を決定
  uint32_t timerColor = (negativeList[currentFace] == 1) ? COLOR_RED : COLOR_WHITE;
  
  // タイマー表示（大きく中央に）
  M5.Display.setTextSize(6);
  M5.Display.setTextColor(timerColor);
  M5.Display.setTextDatum(middle_center);
  String timeStr = formatTime(dig01, dig02, dig03, dig04);
  M5.Display.drawString(timeStr, centerX, centerY);
  
  // ステータス表示
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(bottom_center);
  String status = statusFlg ? "RUNNING" : "STOPPED";
  M5.Display.drawString(status, centerX, screenH - 60);
  
  // ボタンヘルプ
  M5.Display.setTextSize(2);
  if (currentFace == 1 || currentFace == 3) { // Player2またはPlayer4
    M5.Display.drawString("A:Reset", centerX, screenH - 35);
    M5.Display.drawString("B:Start/Stop", centerX, screenH - 15);
  } else {
    M5.Display.drawString("A:Reset  B:Start/Stop", centerX, screenH - 20);
  }
}

void updateTimerOnly() {
  // 回転後の画面サイズを取得
  int screenW = M5.Display.width();
  int screenH = M5.Display.height();
  int centerX = screenW / 2;
  int centerY = screenH / 2;
  
  // タイマー表示エリアの背景をクリア（部分的に）
  int timerAreaWidth = 200;
  int timerAreaHeight = 60;
  M5.Display.fillRect(centerX - timerAreaWidth/2, centerY - timerAreaHeight/2, 
                      timerAreaWidth, timerAreaHeight, COLOR_BLUE);
  
  // タイマー表示の色を決定
  uint32_t timerColor = (negativeList[currentFace] == 1) ? COLOR_RED : COLOR_WHITE;
  
  // タイマー表示のみ再描画
  M5.Display.setTextSize(6);
  M5.Display.setTextColor(timerColor);
  M5.Display.setTextDatum(middle_center);
  String timeStr = formatTime(dig01, dig02, dig03, dig04);
  M5.Display.drawString(timeStr, centerX, centerY);
}

void drawTimerDisplay() {
  // 互換性のため、全体描画を呼び出し
  drawTimerDisplayFull();
}

void drawSetupModeFull() {
  M5.Display.fillScreen(COLOR_BLUE);
  M5.Display.setRotation(1); // 設定画面は常に正面向き
  
  // バッテリー情報表示（右上）
  drawBatteryInfo();
  
  // 設定モード表示
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(top_center);
  M5.Display.drawString("SETUP", 160, 30);
  
  // 時間設定表示
  M5.Display.setTextSize(6);
  String timeStr = formatTime(dig01, dig02, dig03, dig04);
  
  // 点滅処理
  if (!blinkState) {
    switch (updatePosition) {
      case 0: timeStr[0] = ' '; break;  // 分の十の位
      case 1: timeStr[1] = ' '; break;  // 分の一の位
      case 2: timeStr[3] = ' '; break;  // 秒の十の位
      case 3: timeStr[4] = ' '; break;  // 秒の一の位
    }
  }
  
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(timeStr, 160, 120);

  // ボタンヘルプ
  M5.Display.setTextSize(2);
  M5.Display.setTextDatum(bottom_center);
  M5.Display.drawString("A:+1  B:Start  C:Next", 160, 200);
}

void updateSetupTimerOnly() {
  // 時間表示エリアの背景をクリア（部分的に）
  int timerAreaWidth = 240;
  int timerAreaHeight = 50;
  M5.Display.fillRect(160 - timerAreaWidth/2, 120 - timerAreaHeight/2, 
                      timerAreaWidth, timerAreaHeight, COLOR_BLUE);
  
  // 時間設定表示のみ再描画
  M5.Display.setTextSize(6);
  String timeStr = formatTime(dig01, dig02, dig03, dig04);
  
  // 点滅処理
  if (!blinkState) {
    switch (updatePosition) {
      case 0: timeStr[0] = ' '; break;  // 分の十の位
      case 1: timeStr[1] = ' '; break;  // 分の一の位
      case 2: timeStr[3] = ' '; break;  // 秒の十の位
      case 3: timeStr[4] = ' '; break;  // 秒の一の位
    }
  }
  
  M5.Display.setTextColor(COLOR_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(timeStr, 160, 120);
}

void drawSetupMode() {
  // 互換性のため、全体描画を呼び出し
  drawSetupModeFull();
}

void drawBatteryInfo() {
  // 現在の画面サイズを取得
  int screenW = M5.Display.width();
  
  // バッテリー情報を取得
  bool isCharging = M5.Power.isCharging();
  int batteryLevel = M5.Power.getBatteryLevel();
  
  // 前回の表示をクリア（部分更新で省電力化）
  M5.Display.fillRect(screenW - 50, 5, 45, 20, COLOR_BLUE);
  
  // バッテリー残量に応じて色を変更
  uint32_t batteryColor = COLOR_WHITE;
  if (batteryLevel <= 20) {
    batteryColor = COLOR_RED;
  } else if (batteryLevel <= 50) {
    batteryColor = COLOR_YELLOW;
  } else {
    batteryColor = COLOR_GREEN;
  }
  
  // 充電中の場合は色を変更
  if (isCharging) {
    batteryColor = COLOR_GREEN;
  }
  
  // バッテリー表示（右上）
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(batteryColor);
  M5.Display.setTextDatum(top_right);
  
  // 文字列生成の最適化
  static String batteryText; // 静的変数で文字列再利用
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
  // 画面が消灯している場合、ボタン操作で画面を復帰
  if (!isDisplayOn && (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed())) {
    turnOnDisplay();
    needsFullRedraw = true;
    needsBatteryUpdate = true;
    // ボタン処理は次回のループで実行（画面復帰を優先）
    return;
  }
  
  if (M5.BtnB.wasPressed()) {
    if (!statusFlg) {
      // タイマー開始
      statusFlg = true;
      startFlg = true;
      warningPlayed = false; // 開始時に警告フラグリセット
      lastTimerUpdate = millis();
      needsFullRedraw = true; // 開始時は全体再描画
      needsBatteryUpdate = true; // 開始時はバッテリー更新
      
      // 開始時に現在の向きを検出して面を設定
      int orientation = detectOrientation();
      if (orientation >= 0) {
        currentFace = orientation;
      } else {
        currentFace = 4; // デフォルトでP5面
      }
    } else {
      // タイマー停止
      statusFlg = false;
      needsFullRedraw = true; // 停止時も全体再描画（ステータス変更）
      needsBatteryUpdate = true; // 停止時もバッテリー更新
      if (isDisplayOn) {
        drawTimerDisplayFull(); // 停止時に画面を更新
      }
    }
  }
  
  if (M5.BtnA.wasPressed()) {
    if (!startFlg) {
      // 設定モード：値を増加
      incrementDigit();
    } else if (!statusFlg) {
      // 停止中：リセット
      resetAllTimers();
    }
  }
  
  if (M5.BtnC.wasPressed()) {
    if (!startFlg) {
      // 設定モード：次の桁へ
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
  if (accelY >= ACCEL_THRESHOLD) return 0; // P1面（Y軸正）
  if (accelX >= ACCEL_THRESHOLD) return 1; // P2面（X軸正）
  if (accelY <= -ACCEL_THRESHOLD) return 2; // P3面（Y軸負）
  if (accelX <= -ACCEL_THRESHOLD) return 3; // P4面（X軸負）
  if (accelZ >= ACCEL_THRESHOLD) return 4; // P5面（Z軸正）
  return -1; // 該当なし
}

void playTone(int frequency, int duration) {
  M5.Speaker.tone(frequency, duration);
}

void resetAllTimers() {
  statusFlg = false;
  startFlg = false;
  currentFace = 4; // P5面に戻る
  warningPlayed = false; // 警告フラグもリセット
  needsFullRedraw = true; // リセット時も全体再描画
  needsBatteryUpdate = true; // リセット時もバッテリー更新
  
  // 画面が消灯している場合は復帰
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
  
  // 設定値を保存
  initDig01 = dig01;
  initDig02 = dig02;
  initDig03 = dig03;
  initDig04 = dig04;
  
  // 全タイマーに反映
  int totalTime = dig01 * 600 + dig02 * 60 + dig03 * 10 + dig04;
  for (int i = 0; i < 5; i++) {
    timeList[i] = totalTime;
    negativeList[i] = 0;
  }
}

void turnOffDisplay() {
  if (isDisplayOn) {
    M5.Display.setBrightness(0); // 画面を消灯
    isDisplayOn = false;
  }
}

void turnOnDisplay() {
  if (!isDisplayOn) {
    M5.Display.setBrightness(128); // 画面を復帰（50%の明るさ）
    isDisplayOn = true;
  }
}

void nextDigitPosition() {
  updatePosition = (updatePosition < 3) ? updatePosition + 1 : 0;
}