/*
  電子相框
*/
#include "./inc/bitmap.h"
#include <Wire.h>
#include <Flag_UI.h>
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
Flag_Switch prevBtn(18, INPUT_PULLDOWN);
Flag_Switch nextBtn(19, INPUT_PULLDOWN);

// UI 會用到的參數
uint8_t currentPage;
bool btnNextPressed;
bool btnPrevPressed;

Flag_UI_Bitmap banner = Flag_UI_Bitmap(
  0, 0, 128, 16, WHITE,bitmap_banner_background
);
Flag_UI_Bitmap btnL = Flag_UI_Bitmap(
  0, 26, 16, 27, WHITE, bitmap_left_btn
);
Flag_UI_Bitmap btnR = Flag_UI_Bitmap(
  112, 26, 16, 27, WHITE, bitmap_right_btn
);
Flag_UI_Text flashTxt = Flag_UI_Text(
  35, 0, 2, BLACK, WHITE, "Flash"
);
Flag_UI_Text heartTxt = Flag_UI_Text(
  35, 0, 2, BLACK, WHITE, "Heart"
);
Flag_UI_Text starTxt = Flag_UI_Text(
  35, 0, 2, BLACK, WHITE, "Star"
);
Flag_UI_Bitmap flashPic = Flag_UI_Bitmap(
  42, 18, 44, 44, WHITE, bitmap_flash
);
Flag_UI_Bitmap heartPic = Flag_UI_Bitmap(
  42, 18, 44, 44, WHITE, bitmap_heart
);
Flag_UI_Bitmap starPic = Flag_UI_Bitmap(
  42, 18, 44, 44, WHITE, bitmap_star
);
Flag_UI_Page page1, page2, page3;
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
  currentPage = 1;
  btnNextPressed = false;
  btnPrevPressed = false;
  
  // 第 1 頁
  page1.addWidget(&banner); page1.addWidget(&flashTxt);
  page1.addWidget(&btnR);   page1.addWidget(&flashPic);

  // 第 2 頁
  page2.addWidget(&banner); page2.addWidget(&heartTxt);
  page2.addWidget(&btnL);   page2.addWidget(&btnR);
  page2.addWidget(&heartPic);

  // 第 3 頁
  page3.addWidget(&banner); page3.addWidget(&starTxt);
  page3.addWidget(&btnL);   page3.addWidget(&starPic);
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
    case 1: page1.show(); break;
    case 2: page2.show(); break;
    case 3: page3.show(); break;
  }
}
