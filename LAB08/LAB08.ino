/*
  讀取按壓開關狀態
*/
#include <Flag_Switch.h>

#define LED_ON  0
#define LED_OFF 1
#define INPUT_PIN_NUM 12

Flag_Switch btn(INPUT_PIN_NUM, INPUT);

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
}

void loop() {
  if(btn.read())  digitalWrite(LED_BUILTIN, LED_ON);
  else            digitalWrite(LED_BUILTIN, LED_OFF);
}
