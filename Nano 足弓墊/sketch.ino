// Analog sensor pins
const int HEEL_PIN = A0;       // pot1 → 足跟 FSR
const int ARCH_PIN = A1;       // pot2 → 足弓 FSR
const int FORE_PIN = A2;       // pot3 → 前掌 FSR

// Output pins
const int PUMP_RELAY_PIN = 6;  // Relay IN → D6（氣泵開關）
const int PUMP_LED_PIN   = 7;  // Pump LED → D7（氣泵狀態指示燈）
const int LOW_LED_PIN    = 5;  // Low LED  → D5（足弓調整指示燈 / 建議增加支撐）
const int HIGH_LED_PIN   = 4;  // High LED → D4（壓力過高 / 建議降低支撐）

// ADC 最大值（0~1023）
const float ADC_MAX = 1023.0;

// 手機 APP 使用的「百分比條件」（0~100）
const float ARCH_INFLATE_PCT = 25.0; // 足弓壓力低
const float AVG_PCT  = 45.0; // 平均壓力

// 將 0~1023 轉成 0~100 百分比
float toPercent(int raw) {
  return (raw * 100.0) / ADC_MAX;
}

void setup() {
  Serial.begin(115200);

  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(PUMP_LED_PIN, OUTPUT);
  pinMode(LOW_LED_PIN, OUTPUT);
  pinMode(HIGH_LED_PIN, OUTPUT);

  // 初始全部關閉
  digitalWrite(PUMP_RELAY_PIN, LOW);
  digitalWrite(PUMP_LED_PIN, LOW);
  digitalWrite(LOW_LED_PIN, LOW);
  digitalWrite(HIGH_LED_PIN, LOW);
}

void loop() {
  // 讀取三區壓力值（0~1023）
  int heelRaw = analogRead(HEEL_PIN);
  int archRaw = analogRead(ARCH_PIN);
  int foreRaw = analogRead(FORE_PIN);

  // 轉為 0~100 百分比（與手機 APP 同一範圍）
  float heelPct = toPercent(heelRaw);
  float archPct = toPercent(archRaw);
  float forePct = toPercent(foreRaw);
  float avgPct  = (heelPct + archPct + forePct) / 3.0;

  // 輸出 JSON（仍輸出「實際 0~1023」數值，之後 Flutter 要換成硬體模式時可直接解析）
  Serial.print("{\"heel\":");
  Serial.print(heelRaw);
  Serial.print(",\"arch\":");
  Serial.print(archRaw);
  Serial.print(",\"fore\":");
  Serial.print(foreRaw);
  Serial.print("}");

  // === 條件判斷邏輯 ===
  bool shouldInflate = (archPct < ARCH_INFLATE_PCT) &&
                       (avgPct  >= AVG_PCT)  &&
                       (heelPct >= AVG_PCT);

  bool shouldDeflate = (avgPct  >= AVG_PCT)  &&
                       (heelPct >= AVG_PCT)  &&
                       (archPct >= avgPct);

  bool abnormal = (40 <= avgPct && avgPct <= 60)  &&
                  (50 <= heelPct && heelPct <= 70)  &&
                  (25 <= archPct && archPct <= 35);


  // 狀態輸出說明用
  if (shouldInflate) {
    // 對應「增加支撐」→ 啟動氣泵
    digitalWrite(PUMP_RELAY_PIN, HIGH); // Relay ON → 氣泵啟動
    digitalWrite(PUMP_LED_PIN, HIGH);   // 氣泵指示燈 ON
    digitalWrite(LOW_LED_PIN, HIGH);    // 顯示為：目前系統正在執行「增加支撐」
    digitalWrite(HIGH_LED_PIN, LOW);    // 高壓燈 OFF

    Serial.println(" -> INFLATE (增加足弓支撐)");
  }
  else if (shouldDeflate) {
    // 對應「降低支撐」→ 停止打氣，並亮高壓燈作為調整提醒
    digitalWrite(PUMP_RELAY_PIN, LOW);  // 停止打氣（DEFLATE 在實際設計可由排氣閥處理）
    digitalWrite(PUMP_LED_PIN, LOW);
    digitalWrite(LOW_LED_PIN, LOW);
    digitalWrite(HIGH_LED_PIN, HIGH);   // 用高壓燈代表目前建議「降低支撐」

    Serial.println(" -> DEFLATE (降低足弓支撐建議)");
  }
  else if (!abnormal) {
    // 異常狀況
    digitalWrite(PUMP_RELAY_PIN, HIGH); 
    digitalWrite(PUMP_LED_PIN, HIGH);
    digitalWrite(LOW_LED_PIN, HIGH);
    digitalWrite(HIGH_LED_PIN, HIGH);   

    Serial.println(" -> Abnormal (足弓出現異常)");
  }
  else {
    // KEEP：維持現狀
    digitalWrite(PUMP_RELAY_PIN, LOW);
    digitalWrite(PUMP_LED_PIN, LOW);
    digitalWrite(LOW_LED_PIN, LOW);
    digitalWrite(HIGH_LED_PIN, LOW);

    Serial.println(" -> KEEP (維持目前設定)");
  }

  delay(200); // 0.2 秒更新一次（與手機 APP 模擬節奏相近）
}
