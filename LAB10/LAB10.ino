/*
  每日食物攝取紀錄--IFTTT
*/
#include "./inc/ifttt.h"
#include "./inc/bitmap.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Flag_Switch.h>
#include <Flag_Model.h>
#include <Flag_HX711.h>
#include <WiFiClientSecure.h>

#define AP_SSID    "Xperia XZ Premium_db49"
#define AP_PWD     "12345678"
#define IFTTT_URL  "https://maker.ifttt.com/trigger/https_test/with/key/dg9J7YHL0mMuDaaRGs9pNU"

#define PAGE_TOTAL 5
enum{ RICE, FISH, MILK, VEGETABLE, FRUIT };

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
Flag_Switch btnPage(18, INPUT_PULLDOWN);
Flag_Switch btnRec(19, INPUT_PULLDOWN);
Flag_HX711 hx711(32, 33); // SCK, DT

// 連網會用到的參數
WiFiClientSecure client;
const char* ssid = AP_SSID;
const char* password = AP_PWD;
const char* SERVER = "ifttt.com";
String encodeTbl[] = {
  "%e5%85%a8%e7%a9%80%e9%9b%9c%e7%b3%a7%e9%a1%9e",
  "%e8%9b%8b%e8%b1%86%e9%ad%9a%e8%82%89%e9%a1%9e",
  "%e4%b9%b3%e5%93%81%e9%a1%9e",
  "%e8%94%ac%e8%8f%9c%e9%a1%9e",
  "%e6%b0%b4%e6%9e%9c%e9%a1%9e"
};

// UI 會用到的參數
uint8_t currentPage;
bool btnPagePressed;
bool btnRecPressed;
// -------------------------------

// 傳送 LINE 訊息
void notify(uint8_t item, float value){
  String ifttt_url = IFTTT_URL;

  Serial.println("\n開始連接伺服器…");
  if(!client.connect(SERVER, 443)){
    Serial.println("連線失敗～");
  }else{
    Serial.println("連線成功！");
    String https_get = "GET " + \
                       ifttt_url + \
                       "?value1=" + encodeTbl[item] + \
                       "&value2=" + String(value, 1) + \
                       " HTTP/1.1\n" + \
                       "Host: " + String(SERVER) + "\n" + \
                       "Connection: close\n\n";
                       
    client.print(https_get);

    // 接收並顯示伺服器的回應
    while(client.connected()){
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("收到HTTPS回應：");
        break;
      }
    }
    
    while(client.available()){
      char c = client.read();
      if(c != 0xFF) Serial.print(c);
      if(c == 0xFF) Serial.println();
    }

    // 關閉連線
    client.stop();
  }
}

//------所有頁面共用的小工具------
void utilities(uint8_t item, float weight, float *total){
  // 當前重量   
  display.setTextSize(1);
  display.setCursor(48, 20);
  display.setTextColor(WHITE, BLACK);
  display.print("Current:");
  display.setCursor(48, 30);
  display.print(weight, 1);
  display.print("g");

  // 總重量   
  display.setTextSize(1);
  display.setCursor(48, 44);
  display.setTextColor(WHITE, BLACK);
  display.print("Total:");
  display.setCursor(48, 54);
  display.print(*total, 1);
  display.print("g");

  // 偵測記錄按鈕
  if(btnRec.read()){
    if(!btnRecPressed) {
      btnRecPressed = true;
      *total += weight;
      // 發 LINE 訊息
      if(*total){
        notify(item, *total);
      }
    }
  }else{
    btnRecPressed = false;
  }
}

//------第 1 頁------
void drawPage1(float weight){
  static float total = 0;
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
  display.setCursor(40, 0);
  display.setTextColor(BLACK, WHITE);
  display.println("Rice");

  // 第 1 頁的照片
  display.drawBitmap(0, 18, bitmap_rice, 44, 44, WHITE);
  
  // 調用小工具
  utilities(RICE, weight, &total);
  
  // 將畫面顯示出來
  display.display();    
}

//------第 2 頁------
void drawPage2(float weight){
  static float total = 0;
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
  display.setCursor(40, 0);
  display.setTextColor(BLACK, WHITE);
  display.println("Fish");

  // 第 2 頁的照片
  display.drawBitmap(0, 18, bitmap_fish, 44, 44, WHITE);
 
  // 調用小工具
  utilities(FISH, weight, &total);

  // 將畫面顯示出來
  display.display();
}

//------第 3 頁------
void drawPage3(float weight){
  static float total = 0;
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
  display.setCursor(40, 0);
  display.setTextColor(BLACK, WHITE);
  display.println("Milk");

  // 第 3 頁的照片
  display.drawBitmap(0, 18, bitmap_milk, 44, 44, WHITE);

  // 調用小工具
  utilities(MILK, weight, &total);

  // 將畫面顯示出來
  display.display();
}

//------第 4 頁------
void drawPage4(float weight){
  static float total = 0;
  display.clearDisplay();

  // 設定各元件參數
  // 第 4 頁的橫幅背景 
  display.drawBitmap(
    0, 0,
    bitmap_banner_background,
    128, 16,
    WHITE
  );    
  
  // 第 4 頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(10, 0);
  display.setTextColor(BLACK, WHITE);
  display.println("Vegetable");

  // 第 4 頁的照片
  display.drawBitmap(0, 18, bitmap_vegetable, 44, 44, WHITE);

  // 調用小工具
  utilities(VEGETABLE, weight, &total);

  // 將畫面顯示出來
  display.display();
}

//------第 5 頁------
void drawPage5(float weight){
  static float total = 0;
  display.clearDisplay();

  // 設定各元件參數
  // 第 5 頁的橫幅背景 
  display.drawBitmap(
    0, 0,
    bitmap_banner_background,
    128, 16,
    WHITE
  );    
  
  // 第 5 頁的橫幅文字
  display.setTextSize(2);
  display.setCursor(35, 0);
  display.setTextColor(BLACK, WHITE);
  display.println("Fruit");

  // 第 5 頁的照片
  display.drawBitmap(0, 18, bitmap_fruit, 44, 44, WHITE);

  // 調用小工具
  utilities(FRUIT, weight, &total);

  // 將畫面顯示出來
  display.display();
}

void setup(){
  // 序列埠設置
  Serial.begin(115200);

  // HX711 初始化
  hx711.begin();

  // 歸零調整
  hx711.tare();

  // Wi-Fi 設置
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n成功連上基地台!");

  // 設置CA憑證  
  client.setCACert(root_ca);
  
  // OLED 初始化
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("OLED初始化失敗, 請重置~");
    while(1);
  }

  // UI 參數初始化
  currentPage = 1;
  btnPagePressed = false;
  btnRecPressed = false;

  // -------------------------- 建構模型 --------------------------
  // 讀取已訓練好的模型檔
  model.begin("/weight_model.json");
}

void loop(){
  // -------------------------- 即時預測 --------------------------
  // 測試資料預處理
  float test_feature_data = (hx711.getWeightAsync() - model.mean) / model.sd;
   
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
  float predictVal;

  // 模型預測
  test_output_tensor = model.predict(&test_feature_tensor);
  
  // 頁面選擇按鈕
  if(btnPage.read()){
    if(currentPage <= PAGE_TOTAL && !btnPagePressed) {
      currentPage++;
      if(currentPage == PAGE_TOTAL + 1) currentPage = 1;
      btnPagePressed = true;
    }
  }else{
    btnPagePressed = false;
  }
  
  // 輸出預測結果
  Serial.print("重量: ");
  model.getResult(test_output_tensor, model.labelMaxAbs, &predictVal);
  Serial.print(predictVal, 1); 
  Serial.println('g');
  
  switch(currentPage - 1){
    case RICE:      drawPage1(predictVal); break;
    case FISH:      drawPage2(predictVal); break;
    case MILK:      drawPage3(predictVal); break;
    case VEGETABLE: drawPage4(predictVal); break;
    case FRUIT:     drawPage5(predictVal); break;
  }
}
