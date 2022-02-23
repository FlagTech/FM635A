/*
  PWM功能
*/
#define BITS 10
#define INVERT(x) ((1 << BITS) - 1 - x)
#define DUTY(x)   ((int)((x / 100.0) * ((1 << BITS) - 1)))

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  ledcSetup(0, 5000, BITS);        // 設定PWM，通道0、5KHz、10位元
  ledcAttachPin(LED_BUILTIN, 0);   // 指定內建的LED接腳成PWM輸出
}

void loop() {
  // Duty cycle於0% ~ 50%往復調整
  for(int i = DUTY(0); i < DUTY(50); i++){
    ledcWrite(0, INVERT(i)); //內建的LED接腳為LOW致動, 所以須將電壓反向
    delay(2);
  }

  for(int i = DUTY(50); i > DUTY(0); i--){
    ledcWrite(0, INVERT(i)); //內建的LED接腳為LOW致動, 所以須將電壓反向
    delay(2);
  }
}
