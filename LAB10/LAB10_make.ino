/*
  每日飲食攝取紀錄--MAKE
*/
#include "./inc/bitmap.h"
#include <Wire.h>
#include <Flag_UI.h>
#include <Flag_Switch.h>
#include <Flag_Model.h>
#include <Flag_HX711.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>

#define AP_SSID    "基地台SSID"
#define AP_PWD     "基地台密碼"
#define IFTTT_URL  "MAKE請求路徑"

enum{RICE, FISH, MILK, VEGETABLE, FRUIT, PAGE_TOTAL};

// ------------全域變數------------
// 神經網路模型
Flag_Model model; 

// OLED 物件
Adafruit_SSD1306 display(
  128,  // 螢幕寬度
  64,   // 螢幕高度
  &Wire // 指向 Wire 物件的指位器, I2C 通訊用
);

// 感測器的物件
Flag_Switch btnPage(18);
Flag_Switch btnRec(19);
Flag_HX711  hx711(32, 33); // SCK, DT

// UI 會用到的參數
uint8_t currentPage;
bool btnPagePressed;
bool btnRecPressed;
float currentWeight;
float totalWeight[PAGE_TOTAL];

Flag_UI_Bitmap banner = Flag_UI_Bitmap(
  0, 0, 128, 16, bitmap_banner_background
);
Flag_UI_Text txt[] = { 
  Flag_UI_Text(40, 0, 2, BLACK, WHITE, "Rice"),
  Flag_UI_Text(40, 0, 2, BLACK, WHITE, "Fish"),
  Flag_UI_Text(40, 0, 2, BLACK, WHITE, "Milk"),
  Flag_UI_Text(10, 0, 2, BLACK, WHITE, "Vegetable"),
  Flag_UI_Text(35, 0, 2, BLACK, WHITE, "Fruit"),
};
Flag_UI_Bitmap bitmap[] = {
  Flag_UI_Bitmap(0, 18, 44, 44, bitmap_rice),
  Flag_UI_Bitmap(0, 18, 44, 44, bitmap_fish),
  Flag_UI_Bitmap(0, 18, 44, 44, bitmap_milk),
  Flag_UI_Bitmap(0, 18, 44, 44, bitmap_vegetable),
  Flag_UI_Bitmap(0, 18, 44, 44, bitmap_fruit),
};
Flag_UI_Text weightLabel = Flag_UI_Text(
  48, 20, 1, WHITE, BLACK, "Current:"
);
Flag_UI_Text weightTxt = Flag_UI_Text(
  48, 30, 1, WHITE, BLACK, &currentWeight, 1, "g"
);
Flag_UI_Text totalWeightLabel = Flag_UI_Text(
  48, 44, 1, WHITE, BLACK, "Total:"
);
Flag_UI_Text totalWeightTxt[] = {
  Flag_UI_Text(
    48, 54, 1, WHITE, BLACK, &totalWeight[RICE], 1, "g"
  ),
  Flag_UI_Text(
    48, 54, 1, WHITE, BLACK, &totalWeight[FISH], 1, "g"
  ),
  Flag_UI_Text(
    48, 54, 1, WHITE, BLACK, &totalWeight[MILK], 1, "g"
  ),
  Flag_UI_Text(
    48, 54, 1, WHITE, BLACK, 
    &totalWeight[VEGETABLE], 1, "g"
  ),
  Flag_UI_Text(
    48, 54, 1, WHITE, BLACK, &totalWeight[FRUIT], 1,"g"
  ),
};
Flag_UI_Page page[PAGE_TOTAL];
// -------------------------------

// 傳送 LINE 訊息
void notify(uint8_t page, float totalWeight){
  String str[] = {
    "全穀雜糧類",
    "蛋豆魚肉類",
    "乳品類",
    "蔬菜類",
    "水果類"
  };
  String ifttt_url = IFTTT_URL;
  String url = ifttt_url + 
    "?value1=" + urlEncode(str[page]) + 
    "&value2=" + String(totalWeight, 1);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if(httpCode < 0) Serial.println("連線失敗");
  else             Serial.println("連線成功");
  http.end();
}

void setup(){
  // 序列埠設置
  Serial.begin(115200);

  // HX711 初始化
  hx711.begin();

  // 扣重調整
  hx711.tare();

  // OLED 初始化
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("OLED初始化失敗, 請重置~");
    while(1);
  }

  // Wi-Fi 設置
  WiFi.begin(AP_SSID, AP_PWD);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n成功連上基地台!");

  // UI 參數初始化
  currentPage = RICE;
  btnPagePressed = false;
  btnRecPressed = false;

  // 第 1 頁 ~ 第 5 頁
  for(uint8_t i = 0; i < PAGE_TOTAL; i++){
    page[i].setDisplay(&display);
    page[i].addWidget(&banner);
    page[i].addWidget(&txt[i]);
    page[i].addWidget(&bitmap[i]);
    page[i].addWidget(&weightLabel);
    page[i].addWidget(&weightTxt);
    page[i].addWidget(&totalWeightLabel); 
    page[i].addWidget(&totalWeightTxt[i]);
  }

  // ----------------- 建構模型 --------------------
  // 讀取已訓練好的模型檔
  model.begin("/weight_model.json");
}

void loop(){
  // ----------------- 即時預測 --------------------
  // 測試資料預處理
  float test_feature_data = 
    (hx711.getWeightAsync() - model.mean) / model.sd;
   
  // 模型預測
  uint16_t test_feature_shape[] = {
    1, // 每次測試一筆資料
    model.inputLayerDim
  }; 
  aitensor_t test_feature_tensor = AITENSOR_2D_F32(
    test_feature_shape, 
    &test_feature_data
  );
  aitensor_t *test_output_tensor;

  test_output_tensor = model.predict(
    &test_feature_tensor
  );
  
  // 輸出預測結果
  float predictVal;
  Serial.print("重量: ");
  model.getResult(
    test_output_tensor, 
    model.labelMaxAbs, 
    &predictVal
  );
  Serial.print(predictVal, 1); 
  Serial.println('g');
  currentWeight = predictVal;

  // 頁面選擇按鈕
  if(btnPage.read()){
    if(currentPage <= FRUIT && !btnPagePressed) {
      currentPage++;
      if(currentPage == PAGE_TOTAL) currentPage = RICE;
      btnPagePressed = true;
    }
  }else{
    btnPagePressed = false;
  }
  
  // 偵測記錄按鈕
  if(btnRec.read()){
    if(!btnRecPressed) {
      btnRecPressed = true;
      totalWeight[currentPage] += currentWeight;
      // 發 LINE 訊息
      notify(currentPage, totalWeight[currentPage]);
    }
  }else{
    btnRecPressed = false;
  }

  // 顯示畫面
  page[currentPage].show();
}
