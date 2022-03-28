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
  Serial.begin(115200);

  if(display.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.setTextColor(WHITE,BLACK);
    display.println("FLAG X AIfES");
    display.display();
  }else{
    Serial.println("OLED初始化失敗, 請重置~");
    while(1);
  }
}

void loop(){}
