#include "FM635A_Utilities.h"

FM635A_Utilities::SwitchWithDebounce btn(12, INPUT);

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, FM635A_Utilities::LED_OFF);
}

void loop() {
  if(btn.read() == HIGH)      digitalWrite(LED_BUILTIN, FM635A_Utilities::LED_ON);
  else if(btn.read() == LOW)  digitalWrite(LED_BUILTIN, FM635A_Utilities::LED_OFF);
  else                        Serial.println("除彈跳中~");
}
