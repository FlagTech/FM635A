/*
  雲端門禁推播系統 -- IFTTT
*/
#include "./inc/ifttt.h"
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_MPU6050.h>
#include <Flag_Switch.h>
#include <ESP32_Servo.h>
#include <WiFiClientSecure.h>

#define LED_ON  0
#define LED_OFF 1
#define LOCK 180
#define UNLOCK 0
#define COLLECT_BTN_PIN 39
#define LOCK_BTN_PIN 34
#define SERVO_PIN 33

// 1個週期(PERIOD)取MPU6050的6個參數(SENSOR_PARA)
// 每10個週期(PERIOD)為一筆特徵資料
#define PERIOD 10
#define SENSOR_PARA 6
#define FEATURE_DIM (PERIOD * SENSOR_PARA)

//------------全域變數------------
// 讀取資料的物件
Flag_DataReader reader;
Flag_DataBuffer *data;

// 神經網路模型
Flag_Model model; 

// 感測器的物件
Flag_MPU6050 mpu6050;
Flag_Switch collectBtn(COLLECT_BTN_PIN, INPUT);
Flag_Switch lockBtn(LOCK_BTN_PIN, INPUT);

// 伺服馬達物件
Servo servo;

// 連網會用到的參數
WiFiClientSecure client;  
const char* ssid = "Xperia XZ Premium_db49";
const char* password = "12345678";
const char* SERVER = "ifttt.com";

// 資料預處理會用到的參數
float mean;
float sd;

// 即時預測會用到的參數
float sensorData[FEATURE_DIM];
uint32_t sensorArrayIndex = 0;
uint32_t collectFinishedCond = 0;
uint32_t lastMeaureTime = 0;
//--------------------------------

// IFTTT通知LINE
void notify(){
  Serial.println("\n開始連接伺服器…");
  if(!client.connect(SERVER, 443)){
    Serial.println("連線失敗～");
  }else{
    Serial.println("連線成功！");
    String https_get = "GET https://maker.ifttt.com/trigger/https_test/with/key/dg9J7YHL0mMuDaaRGs9pNU HTTP/1.1\n"\
                       "Host: " + String(SERVER) + "\n" +\
                       "Connection: close\n\n";
                       
    client.print(https_get);

    while(client.connected()){
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("收到HTTPS回應：");
        break;
      }
    }
    // 接收並顯示伺服器的回應
    while(client.available()){
      char c = client.read();
      if(c != 0xFF) Serial.print(c);
      if(c == 0xFF) Serial.println();
    }
    client.stop();
  }
}

// 檢查密碼 : 213
uint8_t pwdCheck(uint8_t gesture, float threshold){
  // 信心度的臨界值
  #define THRESHOLD_VAL 0.75

  // 狀態
  enum{
    FIRST_WORD,
    SECOND_WORD,
    THIRD_WORD
  };

  // 狀態變數
  static uint8_t state = FIRST_WORD;

  // 狀態機
  switch(state){
    case FIRST_WORD:
      if(gesture == 2 && threshold >= THRESHOLD_VAL) {
        Serial.println();
        Serial.print(2);
        state++;
        return 1; 
      }else{          
        state = FIRST_WORD;
        return 0; 
      }  
      break;

    case SECOND_WORD:
      if(gesture == 1 && threshold >= THRESHOLD_VAL) {
        Serial.print(1);
        state++;
        return 1; 
      }else{            
        state = FIRST_WORD;
        return 0; 
      }  
      break;

    case THIRD_WORD:
      if(gesture == 3 && threshold >= THRESHOLD_VAL) {
        Serial.println(3);
        Serial.println("解鎖成功");
        servo.write(UNLOCK);
        notify();
        state = FIRST_WORD;
        return 1;
      }else{     
        state = FIRST_WORD;  
        return 0;     
      } 
      break;

    default:
      state = FIRST_WORD; 
      return 0;
  }
}

// 檢查密碼 : 閃電 圓形 三角形
uint8_t pwdCheck2(uint8_t gesture, float threshold){
  // 信心度的臨界值
  #define THRESHOLD_VAL 0.75

  // 狀態
  enum{
    FIRST_SHAPE,
    SECOND_SHAPE,
    THIRD_SHAPE
  };

  // 狀態變數
  static uint8_t state = FIRST_SHAPE;

  // 狀態機
  switch(state){
    case FIRST_SHAPE:
      if(gesture == 5 && threshold >=  THRESHOLD_VAL) {
        Serial.print("\n閃電 ");
        state++;
        return 1; 
      }else{          
        state = FIRST_SHAPE;
        return 0; 
      }  
      break;

    case SECOND_SHAPE:
      if(gesture == 4 && threshold >=  THRESHOLD_VAL) {
        Serial.print("圓形 ");
        state++;
        return 1; 
      }else{            
        state = FIRST_SHAPE;
        return 0; 
      }  
      break;

    case THIRD_SHAPE:
      if(gesture == 6 && threshold >=  THRESHOLD_VAL) {
        Serial.println("三角形");
        Serial.println("解鎖成功");
        servo.write(UNLOCK);
        notify();
        state = FIRST_SHAPE;
        return 1;
      }else{     
        state = FIRST_SHAPE;  
        return 0;     
      } 
      break;

    default:
      state = FIRST_SHAPE; 
      return 0;
  }
}

void setup() {
  // UART設置
  Serial.begin(115200);

  // mpu6050設置
  mpu6050.init();
  while(!mpu6050.isReady()); 

  // GPIO設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
  
  // Servo設置
  servo.attach(SERVO_PIN, 500, 2400); // 設定伺服馬達的接腳

  // Wi-Fi設置
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.print("\n成功連上基地台!\nIP位址：");
  Serial.println(WiFi.localIP());

  // 設置CA憑證  
  client.setCACert(root_ca);

  // 多元分類類型的資料讀取
  data = reader.read("/dataset/one.txt,/dataset/two.txt,/dataset/three.txt,/dataset/circle.txt,/dataset/flash.txt,/dataset/triangle.txt", reader.MODE_CATEGORICAL); //注意讀檔案順序分別對應到one-hot encoding

  // 取得特徵資料的平均值
  mean = data->featureMean;

  // 取得特徵資料的標準差
  sd = data->featureSd;

  Serial.println(F("----- 手勢解鎖門禁 -----"));
  Serial.println();

  // -------------------------- 建構模型 --------------------------
  // 讀取已訓練的模型檔
  model.begin("/gesture_model.json");
}

void loop() {
  // -------------------------- 即時預測 --------------------------
  // 當按鈕按下時開始收集資料   
  if(collectBtn.read()){
    // 收集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LED_ON);

    //100ms取一次, 共取10次, 也就是一秒
    if(millis() - lastMeaureTime > 100){
      //mpu6050資料更新  
      mpu6050.update();

      if(collectFinishedCond == PERIOD){
        // 使用訓練好的模型來預測
        float *test_feature_data = sensorData; 
        uint16_t test_feature_shape[] = {1, FEATURE_DIM};
        aitensor_t test_feature_tensor = AITENSOR_2D_F32(test_feature_shape, test_feature_data);
        aitensor_t *test_output_tensor;
        float predictVal[model.getNumOfOutputs()];

        // 測試資料預處理
        for(int i = 0; i < FEATURE_DIM ; i++){
          test_feature_data[i] = (test_feature_data[i] - mean) / sd;
        }

        // 模型預測
        test_output_tensor = model.predict(&test_feature_tensor);
        model.getResult(test_output_tensor, predictVal);

        // 找到機率最大的索引值
        uint8_t indexOfMaxArg = model.argmax(predictVal);
        uint8_t gesture = indexOfMaxArg + 1;
        if((!pwdCheck(gesture, predictVal[indexOfMaxArg])) && 
           (!pwdCheck2(gesture, predictVal[indexOfMaxArg])))
        {
          Serial.println("\n輸入密碼錯誤"); 
        }

        // 當還按著按鈕則阻塞, 直到放開按鈕
        while(collectBtn.read());
        
      }else{
        sensorData[sensorArrayIndex] = mpu6050.data.accX; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.accY; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.accZ; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.gyrX; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.gyrY; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.gyrZ; sensorArrayIndex++;
        collectFinishedCond++;
      }
      lastMeaureTime = millis();
    }
  }else{
    // 按鈕放開, 則代表資料要重新收集
    digitalWrite(LED_BUILTIN, LED_OFF);
    collectFinishedCond = 0;
    sensorArrayIndex = 0;
  }

  // 上鎖
  if(lockBtn.read()){
    servo.write(LOCK);
    delay(15); // 延遲一段時間，讓伺服馬達轉到定位。 
  }
}