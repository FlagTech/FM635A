/*
  電子相框
*/
#include "./inc/bitmap.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Flag_Switch.h>

#define PAGE_TOTAL 3

// ------------全域變數------------
// OLED 物件
Adafruit_SSD1306 display(
  128,  // 螢幕寬度
  64,   // 螢幕高度
  &Wire // 指向 Wire 物件的指位器, I2C 通訊用
);

// 感測器的物件
Flag_Switch nextBtn(39);
Flag_Switch prevBtn(34);

// UI 會用到的參數
uint8_t currentPage;
bool btnNextPressed;
bool btnPrevPressed;
// -------------------------------
  
//------第 1 頁------
void drawPage1(){
  display.clearDisplay();

  // 設定各元件參數
  // 第 1 頁的橫幅背景 
  display.drawBitmap(
    0, 0, 
    bitmap_banner_background, 
    128, 16, 
    WHITE
  );
  
  // 第 1 頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(35, 0);
  display.setTextColor(BLACK, WHITE);
  display.println("Flash");

  // 第 1 頁的右鍵
  display.drawBitmap(112, 26, bitmap_right_btn, 16, 27, WHITE);

  // 第 1 頁的照片
  display.drawBitmap(42, 18, bitmap_flash, 44, 44, WHITE);

  // 顯示畫面
  display.display();
}

//------第 2 頁------
void drawPage2(){
  display.clearDisplay();

  // 設定各元件參數
  // 第 2 頁的橫幅背景 
  display.drawBitmap(
    0, 0, 
    bitmap_banner_background, 
    128, 16, 
    WHITE
  );
  
  // 第 2 頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(35, 0);
  display.setTextColor(BLACK, WHITE);
  display.println("Heart");

  // 第 2 頁的左鍵
  display.drawBitmap(0, 26, bitmap_left_btn, 16, 27, WHITE);

  // 第 2 頁的右鍵
  display.drawBitmap(112, 26, bitmap_right_btn, 16, 27, WHITE);

  // 第 2 頁的照片
  display.drawBitmap(42, 18, bitmap_heart, 44, 44, WHITE); 

  // 顯示畫面
  display.display();
}

//------第 3 頁------
void drawPage3(){
  display.clearDisplay();

  // 設定各元件參數
  // 第 3 頁的橫幅背景 
  display.drawBitmap(
    0, 0, 
    bitmap_banner_background, 
    128, 16, 
    WHITE
  );
  
  // 第 3 頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(45, 0);
  display.setTextColor(BLACK, WHITE);
  display.println("Star");

  // 第 3 頁的左鍵
  display.drawBitmap(0, 26, bitmap_left_btn, 16, 27, WHITE); 

  // 第 3 頁的照片
  display.drawBitmap(42, 18, bitmap_star, 44, 44, WHITE); 

  // 顯示畫面
  display.display();
}

void setup(){
  // 序列埠設置
  Serial.begin(115200);
  
  // OLED 初始化
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("OLED 初始化失敗, 請重置~");
    while(1);
  }

  // UI 初始化
  currentPage = 1;
  btnNextPressed = false;
  btnPrevPressed = false;
}

void loop(){
  // 偵測下一頁按鈕
  if(nextBtn.read()){
    if(currentPage < PAGE_TOTAL && !btnNextPressed) {
      currentPage++;
      btnNextPressed = true;
    }
  }else{
    btnNextPressed = false;
  }

  // 偵測上一頁按鈕
  if(prevBtn.read()){
    if(currentPage > 1 && !btnPrevPressed) {
      currentPage--;
      btnPrevPressed = true;
    }
  }else{
    btnPrevPressed = false;
  }
  
  // 繪製畫面
  switch(currentPage){
    case 1: drawPage1(); break;
    case 2: drawPage2(); break;
    case 3: drawPage3(); break;
  }
}
