/*
  OLED 模組顯示文字
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------全域變數------------
// OLED物件
Adafruit_SSD1306 display(
  128, 
  64, 
  &Wire
);
// -------------------------------

void setup(){
  // 序列埠設置
  Serial.begin(115200);
  
  // OLED 初始化
  Wire.setPins(23,19);
  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    Serial.println("OLED初始化失敗, 請重置~");
    while(1);
  }
  
  // 顯示文字
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.setTextColor(WHITE,BLACK);
  display.println("FLAG X AIfES");
  display.display();
}

void loop(){}
