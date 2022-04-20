/*
  讀取按壓開關狀態
*/
#include <Flag_Switch.h>

// ------------全域變數------------
// 按鈕開關物件
Flag_Switch btn(39);

void setup() {
  // 腳位設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  // 透過按鈕開關控制內建 LED
  if(btn.read())  digitalWrite(LED_BUILTIN, LOW);
  else            digitalWrite(LED_BUILTIN, HIGH);
}
