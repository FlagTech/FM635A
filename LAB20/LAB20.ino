/*
  手勢解鎖門禁 - IFTTT
*/
#include <Flag_Model.h>
#include <Flag_MPU6050.h>
#include <Flag_Switch.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define AP_SSID    "Xperia XZ Premium_db49"
#define AP_PWD     "12345678"
#define IFTTT_URL  "https://maker.ifttt.com/trigger/gesture/with/key/dg9J7YHL0mMuDaaRGs9pNU"

// 定義 0 度解鎖, 90 度上鎖
#define LOCK 90
#define UNLOCK 0

// 1 個週期取 MPU6050 的 6 個參數
// 每 10 個週期為一筆特徵資料
#define SENSOR_PARA 6
#define PERIOD 10

//------------全域變數------------
// 神經網路模型
Flag_Model model; 

// 感測器的物件
Flag_MPU6050 mpu6050;
Flag_Switch collectBtn(19);
Flag_Switch lockBtn(18);

// 伺服馬達物件
Servo servo;

// 即時預測會用到的參數
float sensorData[PERIOD * SENSOR_PARA];
uint32_t sensorArrayIdx;
uint32_t collectFinishedCond;
uint32_t lastMeasTime;
//--------------------------------

// 傳送 LINE 訊息
void notify(){
  String ifttt_url = IFTTT_URL;

  HTTPClient http;
  http.begin(ifttt_url);
  int httpCode = http.GET();
  if(httpCode < 0) Serial.println("連線失敗");
  else             Serial.println("連線成功");
  http.end();
}

// 密碼檢查
void pwdCheck(uint8_t gesture){
  // 密碼 : 213
  uint8_t pwd[] = {2, 1, 3}; 

  // 狀態
  enum{FIRST_WORD,SECOND_WORD,THIRD_WORD,TOTAL_STATE};

  // 狀態變數
  static uint8_t state = FIRST_WORD;

  // 狀態機
  if(gesture == pwd[state]) {
    state++;
    if(state == TOTAL_STATE){
      Serial.println("解鎖成功");
      servo.write(UNLOCK);
      notify();
      state = FIRST_WORD;
    }
  }else{
    Serial.println("輸入密碼錯誤, 請重新輸入");
    state = FIRST_WORD;
  }
}

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // MPU6050 初始化
  mpu6050.init();
  while(!mpu6050.isReady()); 

  // 設定伺服馬達的接腳
  servo.attach(33, 500, 2400);
  servo.write(LOCK);
  
  // 腳位設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Wi-Fi 設置
  WiFi.begin(AP_SSID, AP_PWD);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n成功連上基地台!");

  // 清除蒐集資料會用到的參數
  sensorArrayIdx = 0;
  collectFinishedCond = 0;
  lastMeasTime = 0;

  // ----------------- 建構模型 --------------------
  // 讀取已訓練的模型檔
  model.begin("/gesture_model.json");
}

void loop() {
  // ----------------- 即時預測 --------------------
  // 當按鈕按下時, 開始蒐集資料
  if(collectBtn.read()){
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LOW);

    // 每 100 毫秒為一個週期來取一次 MPU6050 資料
    if(millis() - lastMeasTime > 100){
      // MPU6050 資料更新
      mpu6050.update();

      // 連續取 10 個週期作為一筆特徵資料, 
      // 也就是一秒會取到一筆特徵資料
      if(collectFinishedCond == PERIOD){
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
        aitensor_t test_feature_tensor=AITENSOR_2D_F32(
          test_feature_shape,
          test_feature_data
        );
        aitensor_t *test_output_tensor;
        test_output_tensor = model.predict(
          &test_feature_tensor
        );

        // 輸出預測結果
        float predictVal[model.getNumOfOutputs()];
        model.getResult(
          test_output_tensor,
          predictVal
        );
        Serial.print("預測結果: ");
        model.printResult(predictVal);

        // 找到信心值最大的索引
        uint8_t maxIndex = model.argmax(predictVal);
        Serial.print("你寫的數字為: "); 
        Serial.println(maxIndex + 1);

        // 檢查密碼
        uint8_t gesture = maxIndex + 1;
        if(predictVal[maxIndex] >= 0.70){
          pwdCheck(gesture);
        }else{
          Serial.println("信心值不夠, 不做密碼判定");
        }
        
        // 要先放開按鈕才能再做資料的蒐集
        while(collectBtn.read());
      }else{
        sensorData[sensorArrayIdx] = mpu6050.data.accX;
        sensorArrayIdx++;
        sensorData[sensorArrayIdx] = mpu6050.data.accY;
        sensorArrayIdx++;
        sensorData[sensorArrayIdx] = mpu6050.data.accZ;
        sensorArrayIdx++;
        sensorData[sensorArrayIdx] = mpu6050.data.gyrX;
        sensorArrayIdx++;
        sensorData[sensorArrayIdx] = mpu6050.data.gyrY;
        sensorArrayIdx++;
        sensorData[sensorArrayIdx] = mpu6050.data.gyrZ;
        sensorArrayIdx++;
        collectFinishedCond++;
      }
      lastMeasTime = millis();
    }
  }else{
    // 未蒐集資料時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, HIGH);

    // 按鈕放開, 則代表特徵資料要重新蒐集
    collectFinishedCond = 0;

    // 若中途手放開按鈕, 則不足以形成一筆特徵資料
    sensorArrayIdx = 0;
  }

  // 偵測上鎖按鈕
  if(lockBtn.read()){
    servo.write(LOCK);
    delay(15);
  }
}
