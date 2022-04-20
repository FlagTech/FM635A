/*
  OLED 模組顯示文字
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------全域變數------------
// OLED 物件
Adafruit_SSD1306 display(
  128,  // 螢幕寬度
  64,   // 螢幕高度
  &Wire // 指向 Wire 物件的指位器, I2C 通訊用
);
// -------------------------------

void setup(){
  // 序列埠設置
  Serial.begin(115200);
  
  // OLED 初始化
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("OLED 初始化失敗, 請重置~");
    while(1);
  }

  // 清除畫面
  display.clearDisplay();

  // 設定文字大小
  display.setTextSize(1);

  // 設定游標位置, 即顯示文字的起始位置, (0, 0 是左上角)
  display.setCursor(0, 0);

  // 設定顏色, 第 1 個參數是前景色, 第 2 個參數是背景色
  display.setTextColor(WHITE, BLACK);

  // 設定要顯示的文字：
  display.println("FLAG X AIfES");

  // 顯示畫面：
  display.display();
}

void loop(){}
