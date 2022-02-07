#include "./inc/bitmap.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FM635A_UiLite.h>

#define SCREEN_WIDTH 128
#define SCREEN_HIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HIGHT, &Wire, OLED_RESET);

//------第一頁------
FM635A_UiLite::Page page1;
FM635A_UiLite::ImageView bannerBgOfPage1;
FM635A_UiLite::TextView bannerTxtOfPage1;
FM635A_UiLite::Button rightBtnOfPage1;
FM635A_UiLite::ImageView pictureOfPage1;
void page1Init(){
  //設定各元件參數
  bannerBgOfPage1.config({.id = 0,
                          .x = 0,
                          .y = 0,
                          .width  = 128,
                          .height = 16});
  bannerBgOfPage1.setImg(bitmap_banner_background);

  bannerTxtOfPage1.config({.id = 1,
                           .x = 35,
                           .y = 0});
  bannerTxtOfPage1.setTextSize(2);                      
  bannerTxtOfPage1.setStr("Flash");
  bannerTxtOfPage1.strColorInvertEnable();

  rightBtnOfPage1.config({.id = 2,
                          .x = 112,
                          .y = 26,
                          .width  = 16,
                          .height = 27});
  rightBtnOfPage1.setImg(bitmap_right_btn);
  
  pictureOfPage1.config({.id = 3,
                         .x = 42,
                         .y = 18,
                         .width  = 44,
                         .height = 44});
  pictureOfPage1.setImg(bitmap_flash);

  //將元件加入到頁面
  page1.addWidget((FM635A_UiLite::Widget*)&bannerBgOfPage1);
  page1.addWidget((FM635A_UiLite::Widget*)&bannerTxtOfPage1);
  page1.addWidget((FM635A_UiLite::Widget*)&rightBtnOfPage1);
  page1.addWidget((FM635A_UiLite::Widget*)&pictureOfPage1);
  page1.display = &display;
}

//------第二頁------
FM635A_UiLite::Page page2;
FM635A_UiLite::ImageView bannerBgOfPage2;
FM635A_UiLite::TextView bannerTxtOfPage2;
FM635A_UiLite::Button leftBtnOfPage2;
FM635A_UiLite::Button rightBtnOfPage2;
FM635A_UiLite::ImageView pictureOfPage2;
void page2Init(){
  //設定各元件參數
  bannerBgOfPage2.config({.id = 0,
                          .x = 0,
                          .y = 0,
                          .width  = 128,
                          .height = 16});
  bannerBgOfPage2.setImg(bitmap_banner_background);

  bannerTxtOfPage2.config({.id = 1,
                           .x = 35,
                           .y = 0});
  bannerTxtOfPage2.setTextSize(2);                      
  bannerTxtOfPage2.setStr("Heart");
  bannerTxtOfPage2.strColorInvertEnable();

  leftBtnOfPage2.config({.id = 2,
                         .x = 0,
                         .y = 26,
                         .width  = 16,
                         .height = 27});
  leftBtnOfPage2.setImg(bitmap_left_btn);

  rightBtnOfPage2.config({.id = 3,
                          .x = 112,
                          .y = 26,
                          .width  = 16,
                          .height = 27});
  rightBtnOfPage2.setImg(bitmap_right_btn);
  
  pictureOfPage2.config({.id = 4,
                         .x = 42,
                         .y = 18,
                         .width  = 44,
                         .height = 44});
  pictureOfPage2.setImg(bitmap_heart);

  //將元件加入到頁面
  page2.addWidget((FM635A_UiLite::Widget*)&bannerBgOfPage2);
  page2.addWidget((FM635A_UiLite::Widget*)&bannerTxtOfPage2);
  page2.addWidget((FM635A_UiLite::Widget*)&leftBtnOfPage2);
  page2.addWidget((FM635A_UiLite::Widget*)&rightBtnOfPage2);
  page2.addWidget((FM635A_UiLite::Widget*)&pictureOfPage2);
  page2.display = &display;
}

//------第三頁------
FM635A_UiLite::Page page3;
FM635A_UiLite::ImageView bannerBgOfPage3;
FM635A_UiLite::TextView bannerTxtOfPage3;
FM635A_UiLite::Button leftBtnOfPage3;
FM635A_UiLite::ImageView pictureOfPage3;
void page3Init(){
  //設定各元件參數
  bannerBgOfPage3.config({.id = 0,
                          .x = 0,
                          .y = 0,
                          .width  = 128,
                          .height = 16});
  bannerBgOfPage3.setImg(bitmap_banner_background);

  bannerTxtOfPage3.config({.id = 1,
                           .x = 45,
                           .y = 0});
  bannerTxtOfPage3.setTextSize(2);                      
  bannerTxtOfPage3.setStr("Star");
  bannerTxtOfPage3.strColorInvertEnable();

  leftBtnOfPage3.config({.id = 2,
                         .x = 0,
                         .y = 26,
                         .width  = 16,
                         .height = 27});
  leftBtnOfPage3.setImg(bitmap_left_btn);

  pictureOfPage3.config({.id = 3,
                         .x = 42,
                         .y = 18,
                         .width  = 44,
                         .height = 44});
  pictureOfPage3.setImg(bitmap_star);

  //將元件加入到頁面
  page3.addWidget((FM635A_UiLite::Widget*)&bannerBgOfPage3);
  page3.addWidget((FM635A_UiLite::Widget*)&bannerTxtOfPage3);
  page3.addWidget((FM635A_UiLite::Widget*)&leftBtnOfPage3);
  page3.addWidget((FM635A_UiLite::Widget*)&pictureOfPage3);
  page3.display = &display;
}

void setup(){
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    Serial.println("OLED初始化失敗, 請重置~");
    while(1);
  }

  page1Init();
  page2Init();
  page3Init();
}

void loop(){
  page1.show();
  delay(1500);
  page1.clear();

  page2.show();
  delay(1500);
  page2.clear();

  page3.show();
  delay(1500);
  page3.clear();

  page2.show();
  delay(1500);
  page2.clear();
}
