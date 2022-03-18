/*
  每日食物攝取紀錄
*/
#include "./inc/bitmap.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Flag_Switch.h>
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_HX711.h>

#define SCREEN_WIDTH 128
#define SCREEN_HIGHT 64
#define OLED_RESET -1
#define PAGE_TOTAL 5
#define MODE_BTN_PIN_NUM 39
#define COMFIRM_BTN_PIN_NUM 34
#define HX711_DT_PIN_NUM 33
#define HX711_SCK_PIN_NUM 32

// ------------全域變數------------
// 讀取資料的物件
Flag_DataReader reader;
Flag_DataBuffer *data;

// 神經網路模型
Flag_Model model; 

// OLED物件
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HIGHT, &Wire, OLED_RESET);

// 感測器的物件
Flag_Switch btnMode(MODE_BTN_PIN_NUM, INPUT);
Flag_Switch btnConfrim(COMFIRM_BTN_PIN_NUM, INPUT);
Flag_HX711 hx711(HX711_SCK_PIN_NUM, HX711_DT_PIN_NUM);

// 資料預處理會用到的參數
float mean;
float sd;
float labelMaxAbs;

// UI會用到的參數
uint8_t currentPage;
bool btnModePressed;
bool btnConfrimPressed;
// -------------------------------

//------所有頁面共用的小工具------
void utilities(float weight, float *total){
  // current weight   
  display.setTextSize(1);
  display.setCursor(48,20);
  display.setTextColor(WHITE, BLACK);
  display.print("Current:");
  display.setCursor(48,30);
  display.print(weight);
  display.print("g");

  // total weight   
  display.setTextSize(1);
  display.setCursor(48,44);
  display.setTextColor(WHITE, BLACK);
  display.print("Total:");
  display.setCursor(48,54);
  display.print(*total);
  display.print("g");

  // 偵測確認按鈕
  if(btnConfrim.read()){
    if(!btnConfrimPressed) {
      btnConfrimPressed = true;
      *total += weight;
    }
  }else{
    btnConfrimPressed = false;
  }
}

//------第一頁------
void drawPage1(float weight){
  static float total = 0;
  display.clearDisplay();

  // 設定各元件參數
  // 第1頁的橫幅背景 
  display.drawBitmap(0,0,bitmap_banner_background,128,16,WHITE);  

  // 第1頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(40,0);
  display.setTextColor(BLACK,WHITE);
  display.println("Rice");

  // 第1頁的照片
  display.drawBitmap(0,18,bitmap_rice,44,44,WHITE);  
  
  utilities(weight, &total);
  
  // 將畫面顯示出來
  display.display();    
}

//------第二頁------
void drawPage2(float weight){
  static float total = 0;
  display.clearDisplay();

  // 設定各元件參數
  // 第2頁的橫幅背景 
  display.drawBitmap(0,0,bitmap_banner_background,128,16,WHITE);  
  
  // 第2頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(40,0);
  display.setTextColor(BLACK,WHITE);
  display.println("Fish");

  // 第2頁的照片
  display.drawBitmap(0,18,bitmap_fish,44,44,WHITE); 

  utilities(weight, &total);

  // 將畫面顯示出來
  display.display();
}

//------第三頁------
void drawPage3(float weight){
  static float total = 0;
  display.clearDisplay();

  // 設定各元件參數
  // 第3頁的橫幅背景 
  display.drawBitmap(0,0,bitmap_banner_background,128,16,WHITE);  
  
  // 第3頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(40,0);
  display.setTextColor(BLACK,WHITE);
  display.println("Milk");

  // 第3頁的照片
  display.drawBitmap(0,18,bitmap_milk,44,44,WHITE); 

  utilities(weight, &total);

  // 將畫面顯示出來
  display.display();
}

//------第四頁------
void drawPage4(float weight){
  static float total = 0;
  display.clearDisplay();

  // 設定各元件參數
  // 第4頁的橫幅背景 
  display.drawBitmap(0,0,bitmap_banner_background,128,16,WHITE);  
  
  // 第4頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(10,0);
  display.setTextColor(BLACK,WHITE);
  display.println("Vegetable");

  // 第4頁的照片
  display.drawBitmap(0,18,bitmap_vegetable,44,44,WHITE); 

  utilities(weight, &total);

  // 將畫面顯示出來
  display.display();
}

//------第五頁------
void drawPage5(float weight){
  static float total = 0;
  display.clearDisplay();

  // 設定各元件參數
  // 第5頁的橫幅背景 
  display.drawBitmap(0,0,bitmap_banner_background,128,16,WHITE);  
  
  // 第5頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(35,0);
  display.setTextColor(BLACK,WHITE);
  display.println("Fruit");

  // 第5頁的照片
  display.drawBitmap(0,18,bitmap_fruit,44,44,WHITE); 

  utilities(weight, &total);

  // 將畫面顯示出來
  display.display();
}

void setup(){
  // UART設置
  Serial.begin(115200);

  // hx711設置
  hx711.begin();
  hx711.tare();  // 歸零調整
  Serial.print("offset : ");
  Serial.println(hx711.getOffset());
  
  // OLED 初始化
  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    Serial.println("OLED初始化失敗, 請重置~");
    while(1);
  }

  // UI初始化
  currentPage = 1;
  btnModePressed = false;
  btnConfrimPressed = false;
  drawPage1(0);

  // 回歸類型的資料讀取
  data = reader.read("/dataset/weight.txt", reader.MODE_REGRESSION);

  // 取得特徵資料的平均值
  mean = data->featureMean;

  // 取得特徵資料的標準差
  sd = data->featureSd;

  // 取得標籤資料的最大絕對值
  labelMaxAbs = data->labelMaxAbs; 
        
  Serial.println(F("----- 即時預測重量 -----"));
  Serial.println();

  // -------------------------- 建構模型 --------------------------
  // 讀取已訓練的模型檔
  model.begin("/weight_model.json");
}

void loop(){
  // -------------------------- 即時預測 --------------------------
  // 使用訓練好的模型來預測
  float test_feature_data;  
  uint16_t test_feature_shape[] = {1, data->featureDim}; 
  aitensor_t test_feature_tensor = AITENSOR_2D_F32(test_feature_shape, &test_feature_data);
  aitensor_t *test_output_tensor;
  float predictVal;

  // 偵測模式/選擇按鈕
  if(btnMode.read()){
    if(currentPage <= PAGE_TOTAL && !btnModePressed) {
      currentPage++;
      if(currentPage == PAGE_TOTAL + 1) currentPage = 1;
      btnModePressed = true;
    }
  }else{
    btnModePressed = false;
  }
  
  // 測試資料預處理
  test_feature_data = (hx711.getWeight() - mean) / sd;

  // 模型預測
  test_output_tensor = model.predict(&test_feature_tensor);
  
  // 輸出預測結果
  Serial.print("重量: ");
  model.getResult(test_output_tensor, labelMaxAbs, &predictVal);  //因為標籤資料有經過正規化, 所以要將label的比例還原回來
  Serial.print(predictVal); 
  Serial.println('g');

  switch(currentPage){
    case 1:
      // 全穀雜糧類
      drawPage1(predictVal);
      break;
    case 2:
      // 蛋豆魚肉類
      drawPage2(predictVal);
      break;
    case 3:
      // 乳品類
      drawPage3(predictVal);
      break;
    case 4:
      // 蔬菜類
      drawPage4(predictVal);
      break;
    case 5:
      // 水果類
      drawPage5(predictVal);
      break;
  }
}
