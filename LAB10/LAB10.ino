/*
  電子相框
*/
#include "./inc/bitmap.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Flag_Switch.h>

#define SCREEN_WIDTH 128
#define SCREEN_HIGHT 64
#define OLED_RESET -1
#define PAGE_TOTAL 3
#define NEXT_PIN_NUM 39
#define PREV_PIN_NUM 34

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HIGHT, &Wire, OLED_RESET);
Flag_Switch nextBtn(NEXT_PIN_NUM, INPUT);
Flag_Switch prevBtn(PREV_PIN_NUM, INPUT);

uint8_t currentPage;
uint8_t lastPage;
bool btnNextPressed;
bool btnPrevPressed;
  
//------第一頁------
void drawPage1(){
  display.clearDisplay();

  // 設定各元件參數
  // 第1頁的橫幅背景 
  display.drawBitmap(0,0,bitmap_banner_background,128,16,WHITE);  
  
  // 第1頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(35,0);
  display.setTextColor(BLACK,WHITE);
  display.println("Flash"); //ESP32

  // 第1頁的右鍵
  display.drawBitmap(112,26,bitmap_right_btn,16,27,WHITE); 

  // 第1頁的照片
  display.drawBitmap(42,18,bitmap_flash,44,44,WHITE);  

  // 將畫面顯示出來
  display.display();    
}

//------第二頁------
void drawPage2(){
  display.clearDisplay();

  // 設定各元件參數
  // 第2頁的橫幅背景 
  display.drawBitmap(0,0,bitmap_banner_background,128,16,WHITE);  
  
  // 第2頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(35,0);
  display.setTextColor(BLACK,WHITE);
  display.println("Heart"); //ESP32

  // 第2頁的左鍵
  display.drawBitmap(0,26,bitmap_left_btn,16,27,WHITE); 

  // 第2頁的右鍵
  display.drawBitmap(112,26,bitmap_right_btn,16,27,WHITE); 

  // 第2頁的照片
  display.drawBitmap(42,18,bitmap_heart,44,44,WHITE); 

  // 將畫面顯示出來
  display.display();
}

//------第三頁------
void drawPage3(){
  display.clearDisplay();

  // 設定各元件參數
  // 第3頁的橫幅背景 
  display.drawBitmap(0,0,bitmap_banner_background,128,16,WHITE);  
  
  // 第3頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(45,0);
  display.setTextColor(BLACK,WHITE);
  display.println("Star"); //ESP32

  // 第3頁的左鍵
  display.drawBitmap(0,26,bitmap_left_btn,16,27,WHITE); 

  // 第3頁的照片
  display.drawBitmap(42,18,bitmap_star,44,44,WHITE); 

  // 將畫面顯示出來
  display.display();
}

void setup(){
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    Serial.println("OLED初始化失敗, 請重置~");
    while(1);
  }

  lastPage = currentPage = 1;
  btnNextPressed = false;
  btnPrevPressed = false;
}

void loop(){
  if(nextBtn.read()){
    if(currentPage < PAGE_TOTAL && btnNextPressed) {
      currentPage++;
      btnNextPressed = true;
    }
  }else{
    btnNextPressed = false;
  }

  if(prevBtn.read()){
    if(currentPage > 1 && btnPrevPressed) {
      currentPage--;
      btnPrevPressed = true;
    }
  }else{
    btnPrevPressed = false;
  }
  
  //頁面有變更才需要更新畫面
  if(lastPage != currentPage){
    switch(currentPage){
      case 1:
        drawPage1();
        break;
      case 2:
        drawPage2();
        break;
      case 3:
        drawPage3();
        break;
    }
    lastPage = currentPage;
  }
}
