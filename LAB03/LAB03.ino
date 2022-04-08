/*
  使用電子秤模組
*/
#include <Flag_HX711.h>

// ------------全域變數------------
// 感測器的物件
Flag_HX711 hx711(32, 33); // SCK, DT
// -------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // HX711 初始化
  hx711.begin();

  // 歸零調整
  hx711.tare();  
}

void loop() {
  // 顯示重量
  float weight = hx711.getWeight();
  Serial.print("重量: "); 
  Serial.print(weight, 1); 
  Serial.println('g');
}
