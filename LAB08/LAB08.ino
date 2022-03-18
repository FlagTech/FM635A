/*
  讀取按鈕開關狀態
*/
#include <Flag_Switch.h>

Flag_Switch btn(39);

void setup() {
  // UART設置
  Serial.begin(115200);

  // GPIO設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  if(btn.read())  digitalWrite(LED_BUILTIN, LOW);
  else            digitalWrite(LED_BUILTIN, HIGH);
}
