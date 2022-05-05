/*
  電子相框
*/
#include "./inc/bitmap.h"
#include <Wire.h>
#include <Flag_UI.h>
#include <Flag_Switch.h>

enum{FLASH, HEART, STAR, PAGE_TOTAL};

// ------------全域變數------------
// OLED 物件
Adafruit_SSD1306 display(
  128,  // 螢幕寬度
  64,   // 螢幕高度
  &Wire // 指向 Wire 物件的指位器, I2C 通訊用
);

// 感測器的物件
Flag_Switch prevBtn(18, INPUT_PULLDOWN);
Flag_Switch nextBtn(19, INPUT_PULLDOWN);

// UI 會用到的參數
uint8_t currentPage;
bool btnNextPressed;
bool btnPrevPressed;

Flag_UI_Bitmap banner = Flag_UI_Bitmap(
  0, 0, 128, 16, bitmap_banner_background
);
Flag_UI_Bitmap btnL = Flag_UI_Bitmap(
  0, 26, 16, 27, bitmap_left_btn
);
Flag_UI_Bitmap btnR = Flag_UI_Bitmap(
  112, 26, 16, 27, bitmap_right_btn
);
Flag_UI_Text txt[] = { 
  Flag_UI_Text(35, 0, 2, BLACK, WHITE, "Flash"),
  Flag_UI_Text(35, 0, 2, BLACK, WHITE, "Heart"),
  Flag_UI_Text(45, 0, 2, BLACK, WHITE, "Star"),
};
Flag_UI_Bitmap bitmap[] = {
  Flag_UI_Bitmap(42, 18, 44, 44, bitmap_flash),
  Flag_UI_Bitmap(42, 18, 44, 44, bitmap_heart),
  Flag_UI_Bitmap(42, 18, 44, 44, bitmap_star),
};
Flag_UI_Page page[PAGE_TOTAL];
// -------------------------------

void setup(){
  // 序列埠設置
  Serial.begin(115200);
  
  // OLED 初始化
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("OLED 初始化失敗, 請重置~");
    while(1);
  }

  // UI 初始化
  currentPage = FLASH;
  btnNextPressed = false;
  btnPrevPressed = false;
  
  // 第 1 頁 ~ 第 3 頁
  for(uint8_t i = 0; i < PAGE_TOTAL; i++){
    page[i].setDisplay(&display);
    page[i].addWidget(&banner);
    page[i].addWidget(&txt[i]);
    page[i].addWidget(&bitmap[i]);
    if(i == FLASH){
      page[i].addWidget(&btnR);
    }else if(i == HEART){
      page[i].addWidget(&btnL);
      page[i].addWidget(&btnR);
    }else if(i == STAR){
      page[i].addWidget(&btnL);
    }
  }
}

void loop(){
  // 偵測下一頁按鈕
  if(nextBtn.read()){
    if(currentPage < STAR && !btnNextPressed){
      currentPage++;
      btnNextPressed = true;
    }
  }else{
    btnNextPressed = false;
  }

  // 偵測上一頁按鈕
  if(prevBtn.read()){
    if(currentPage > FLASH && !btnPrevPressed) {
      currentPage--;
      btnPrevPressed = true;
    }
  }else{
    btnPrevPressed = false;
  }
  
  // 顯示畫面
  page[currentPage].show();
}
