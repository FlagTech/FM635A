/*
  OLED模組顯示中英文字
*/
#include "./inc/myFont.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HIGHT, &Wire, OLED_RESET);

void setup(){
  Serial.begin(115200);

  if(display.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(18,0);
    display.setTextColor(WHITE,BLACK);
    display.drawBitmap(0,0,str_1,16,16,WHITE);   //用
    display.println("ESP32");                    //ESP32
    display.drawBitmap(80,0,str_4,16,16,WHITE);  //學
    display.drawBitmap(0,16,str_2,16,16,WHITE);  //機
    display.drawBitmap(16,16,str_3,16,16,WHITE); //器
    display.drawBitmap(32,16,str_4,16,16,WHITE); //學
    display.drawBitmap(48,16,str_5,16,16,WHITE); //習
    display.setCursor(0,0);
    display.display();
  }else{
    Serial.println("OLED初始化失敗, 請重置~");
    while(1);
  }
}

void loop(){}
