/*
  雲端跌倒推播系統--IFTTT
*/
#include "./inc/ifttt.h"
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_MPU6050.h>
#include <WiFiClientSecure.h>

#define LED_ON  0
#define LED_OFF 1
#define BUZZER_PIN 32

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

// 連網會用到的參數
WiFiClientSecure client;  
const char* ssid = "Xperia XZ Premium_db49";
const char* password = "12345678";
const char* SERVER = "ifttt.com";

// 資料預處理會用到的參數
float mean;
float sd;

// 評估模型會用到的參數
float sensorData[FEATURE_DIM];
uint32_t sensorArrayIndex = 0;
uint32_t collectFinishedCond = 0;
uint32_t lastMeaureTime = 0;
bool collect = false;
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

void setup() {
  // UART設置
  Serial.begin(115200);

  // mpu6050設置
  mpu6050.init();
  while(!mpu6050.isReady()); 

  // GPIO設置
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
  digitalWrite(BUZZER_PIN, LOW);

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

  // 2元分類類型的資料讀取
  data = reader.read("/dataset/others.txt,/dataset/fall.txt", reader.MODE_BINARY); //注意讀檔案順序分別對應到one-hot encoding

  // 取得特徵資料的平均值
  mean = data->featureMean;

  // 取得特徵資料的標準差
  sd = data->featureSd;
        
  Serial.println(F("----- 即時預測跌倒姿勢 -----"));
  Serial.println();

  // -------------------------- 建構模型 --------------------------
  // 讀取已訓練的模型檔
  model.begin("/fall_model.json");
}

void loop() {
  // -------------------------- 即時預測 --------------------------
  // 偵測是否要開始蒐集資料
  if(millis() - lastMeaureTime > 100 && !collect){
    //mpu6050資料更新  
    mpu6050.update();

    if(mpu6050.data.gyrX > 150   || mpu6050.data.gyrX < -150  ||
       mpu6050.data.gyrY > 150   || mpu6050.data.gyrY < -150  ||
       mpu6050.data.gyrZ > 150   || mpu6050.data.gyrZ < -150  ||
       mpu6050.data.accX > 0.25  || mpu6050.data.accX < -0.25 ||
       mpu6050.data.accY > -0.75 || mpu6050.data.accY < -1.25 ||
       mpu6050.data.accZ > 0.25  || mpu6050.data.accZ < -0.25)
    {
      collect = true;
    }
    lastMeaureTime = millis();
  }

 // 當開始蒐集資料的條件達成時, 開始蒐集
  if(collect){
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LED_ON);

    // 100ms為一個週期來取一次mpu6050資料, 連續取10個週期作為一筆特徵資料, 也就是一秒會取到一筆特徵資料
    if(millis() - lastMeaureTime > 100){
      //mpu6050資料更新  
      mpu6050.update();

      if(collectFinishedCond == PERIOD){
        // 取得一筆特徵資料, 並使用訓練好的模型來預測以進行評估
        float *eval_feature_data = sensorData; 
        uint16_t eval_feature_shape[] = {1, FEATURE_DIM};
        aitensor_t eval_feature_tensor = AITENSOR_2D_F32(eval_feature_shape, eval_feature_data);
        aitensor_t *eval_output_tensor;
        float predictVal;

        // 測試資料預處理
        for(int i = 0; i < FEATURE_DIM ; i++){
          eval_feature_data[i] = (eval_feature_data[i] - mean) / sd;
        }

        // 模型預測
        eval_output_tensor = model.predict(&eval_feature_tensor);
        model.getResult(eval_output_tensor, &predictVal);
        
        // 輸出預測結果
        Serial.print(F("Calculated output: "));
        model.printResult(&predictVal);
  
        // 若為跌倒, 發出警示聲
        if(predictVal == 1){
          for(int i = 0; i < 5; i++){
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
          }
          notify();
        }
       
        collect = false;
        
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
    // 未蒐集資料時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, LED_OFF);

    // 下次蒐集特徵資料時, 要重新蒐集
    collectFinishedCond = 0;
    sensorArrayIndex = 0;
  }
}
