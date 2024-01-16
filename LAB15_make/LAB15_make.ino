/*
  水果熟成分類系統 -- MAKE
*/
#include <Flag_Model.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define AP_SSID    "基地台SSID"
#define AP_PWD     "基地台密碼"
#define MAKE_URL  "MAKE 請求路徑"

//------------全域變數------------
// 神經網路模型
Flag_Model model; 

// 感測器的物件
SparkFun_APDS9960 apds = SparkFun_APDS9960();

// 計時器變數
uint8_t resetTimer;
uint32_t lastTime;

// 未熟香蕉數量
uint32_t unripeCnt;
//--------------------------------

// 傳送 LINE 訊息
void notify(uint32_t unripeTotal){
  String make_url = MAKE_URL;
  String url = make_url + 
    "?value1=" + String(unripeTotal);
  
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if(httpCode < 0) Serial.println("連線失敗");
  else             Serial.println("連線成功");
  http.end();
}

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // 初始化 APDS9960
  while(!apds.init()){
    Serial.println("APDS-9960 初始化錯誤");
  }

  // 啟用 APDS-9960 光感測器
  while(!apds.enableLightSensor()){
    Serial.println("光感測器初始化錯誤");
  }

  // 啟用 APDS-9960 接近感測器
  while(!apds.enableProximitySensor()){
    Serial.println("接近感測器初始化錯誤");
  }
  
  // 腳位設置
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(32, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(32, LOW);

  // Wi-Fi 設置
  WiFi.begin(AP_SSID, AP_PWD);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n成功連上基地台!");

  // ----------------- 建構模型 --------------------
  // 讀取已訓練的模型檔
  model.begin("/banana_model.json");

  // 初始化計時器變數
  resetTimer = false;
  lastTime = millis();

  // 清除未熟香蕉數量
  unripeCnt = 0;

  Serial.println("檢測開始");
}

void loop(){
  // ----------------- 即時預測 --------------------
  float sensorData[model.inputLayerDim]; 
  uint8_t proximity_data = 0;
  uint16_t red_light = 0;
  uint16_t green_light = 0;
  uint16_t blue_light = 0;
  uint16_t ambient_light = 0;

  if(!apds.readProximity(proximity_data)){
    Serial.println("讀取接近值錯誤");
  }

  // 當足夠接近時, 開始蒐集
  if (proximity_data == 255) {
    // 辨識時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LOW);

    if(!apds.readAmbientLight(ambient_light)  ||
       !apds.readRedLight(red_light)          ||
       !apds.readGreenLight(green_light)      ||
       !apds.readBlueLight(blue_light)){
      Serial.println("讀值錯誤");
    }else{
      // 取得各種顏色的比例
      float sum = red_light + green_light + blue_light;
      float redRatio = 0;
      float greenRatio = 0;
      float blueRatio = 0;
      
      if(sum != 0){
        redRatio = red_light / sum;
        greenRatio = green_light / sum;
        blueRatio = blue_light / sum;
      }
     
      sensorData[0] = redRatio;
      sensorData[1] = greenRatio;
      sensorData[2] = blueRatio;

      // 測試資料預處理
      float *test_feature_data = sensorData; 
      for(int i = 0; i < model.inputLayerDim; i++){
        test_feature_data[i] = 
          (sensorData[i] - model.mean) / model.sd;
      }
      
      // 模型預測
      uint16_t test_feature_shape[] = {
        1, // 每次測試一筆資料 
        model.inputLayerDim
      };
      aitensor_t test_feature_tensor = AITENSOR_2D_F32(
        test_feature_shape, 
        test_feature_data
      );
      aitensor_t *test_output_tensor;

      test_output_tensor = model.predict(
        &test_feature_tensor
      );

      // 輸出預測結果
      float predictVal;
      model.getResult(
        test_output_tensor, 
        &predictVal
      );
      Serial.print("預測結果: ");
      Serial.print(predictVal);
      if(predictVal > 0.5) {
        Serial.println(" 已熟成");
      }else{
        unripeCnt++;
        Serial.println(" 未熟成");
      }
      digitalWrite(32, HIGH);  
      delay(300);
      digitalWrite(32, LOW); 
      delay(1000);
      resetTimer = true;
    }
  } else {
    // 未辨識時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, HIGH);
    if(resetTimer){
      lastTime = millis();
      resetTimer = false;
    }

    // 判斷是否檢測完畢
    if(millis() - lastTime > 8000){
      notify(unripeCnt);
      Serial.println("檢測結束");
      while(1);
    }
  }
}
