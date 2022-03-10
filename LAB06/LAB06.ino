/*
  自製電子秤
*/
#include <Flag_HX711.h>

#define HX711_DT_PIN_NUM 33
#define HX711_SCK_PIN_NUM 32

// ------------全域變數------------
// 感測器的物件
Flag_HX711 hx711(HX711_SCK_PIN_NUM, HX711_DT_PIN_NUM);
// -------------------------------

void setup() {
  // UART設置
  Serial.begin(115200);

  // hx711設置
  hx711.begin();
  hx711.tare();  // 歸零調整
  Serial.print("offset : ");
  Serial.println(hx711.getOffset());
}

void loop() {
  // 顯示重量
  float weight = hx711.getWeight();
  Serial.print("重量: "); 
  Serial.print(weight, 1); 
  Serial.println('g');
}
