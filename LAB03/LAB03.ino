/*
  自製電子秤
*/
#include <Flag_HX711.h>

// ------------全域變數------------
// 感測器的物件
Flag_HX711 hx711(32, 33); //sck, dt
// -------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // hx711設置
  hx711.begin();
  // 歸零調整
  hx711.tare();  
  // 若取得的重量為負值可以使用invert反相
  // hx711.invert(); 
}

void loop() {
  // 顯示重量
  float weight = hx711.getWeight();
  Serial.print("重量: "); 
  Serial.print(weight, 1); 
  Serial.println('g');
}
