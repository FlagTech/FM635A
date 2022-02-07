#define BITS 10

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  ledcSetup(0, 5000, BITS);        // 設定PWM，通道0、5KHz、10位元
  ledcAttachPin(LED_BUILTIN, 0);   // 指定內建的LED接腳成PWM輸出
}

void loop() {
  for(int i = 0; i<512 ;i++){
    ledcWrite(0, 1023-i);
    delay(2);
  }

  for(int i = 512; i>0 ;i--){
    ledcWrite(0, 1023-i);
    delay(2);
  }
}
