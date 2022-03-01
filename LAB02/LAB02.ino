/*
  讀取ADC值
*/
#define BITS 10

void setup() {
  Serial.begin(115200);

  analogSetAttenuation(ADC_11db); // 衰減11db, 目的是可以量測到3.3v的輸入電壓
  analogSetWidth(BITS);           // 10位元解析度
}

void loop() {
  uint16_t adc = analogRead(A0);  // 讀取A0腳的類比值
  Serial.printf("A0讀到的值：%u\n", adc);
  delay(1000);
}