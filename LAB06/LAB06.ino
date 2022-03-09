/*
  自製電子秤
*/
#include <Flag_HX711.h>

// ------------全域變數------------
// 感測器的物件
Flag_HX711 hx711(16, 17); //sck pin, dout pin
// -------------------------------

void setup() {
  // UART設置
  Serial.begin(115200);

  // hx711設置
  hx711.tare();  // 歸零調整, 取得offset平均值
  Serial.print("offset : ");
  Serial.println(hx711.getOffset());
}

void loop() {
  // 顯示重量
  Serial.print("重量: "); 
  Serial.print(hx711.getWeight(), 1); 
  Serial.println('g');
}
