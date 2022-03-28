/*
  農產品識別系統
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_DataExporter.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>

#define LED_ON  0
#define LED_OFF 1

// 取APDS9960的3個參數(SENSOR_PARA)為一筆特徵資料
#define SENSOR_PARA 3
#define FEATURE_DIM SENSOR_PARA

//------------全域變數------------
// 讀取資料的物件
Flag_DataReader reader;
Flag_DataBuffer *data;

// 神經網路模型
Flag_Model model; 

// 感測器的物件
SparkFun_APDS9960 apds = SparkFun_APDS9960();

// 訓練用的特徵資料
float *train_feature_data;

// 對應特徵資料的標籤值
float *train_label_data;

// 資料預處理會用到的參數
float mean;
float sd;

// 蒐集資料會用到的參數
float sensorData[FEATURE_DIM]; 
//--------------------------------

void setup() {
  // UART設置
  Serial.begin(115200);

  // 初始化APDS9960
  if(apds.init()) Serial.println(F("APDS-9960初始化完成"));
  else            Serial.println(F("APDS-9960初始化錯誤"));
  
  // 啟用APDS-9960光傳感器
  if(apds.enableLightSensor(false)) Serial.println(F("光傳感器正在運行"));
  else                              Serial.println(F("光傳感器初始化錯誤"));
  
  // 調整接近傳感器增益
  if(!apds.setProximityGain(PGAIN_2X)) Serial.println(F("設置PGAIN時出現問題"));
  
  // 啟用APDS-9960接近傳感器
  if(apds.enableProximitySensor(false)) Serial.println(F("接近傳感器正在運行"));
  else                                  Serial.println(F("傳感器初始化錯誤"));
  
  // GPIO設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  // 多元分類類型的資料讀取
  data = reader.read("/dataset/red.txt,/dataset/blue.txt,/dataset/yellow.txt", reader.MODE_CATEGORICAL);
  
  // 設定訓練用的特徵資料
  train_feature_data = data->feature;

  // 設定對應特徵資料的標籤值
  train_label_data = data->label;

  // 取得特徵資料的平均值
  mean = data->featureMean;

  // 取得特徵資料的標準差
  sd = data->featureSd;
        
  // 特徵資料正規化: 標準差法
  for(int j = 0; j < data->featureDataArryLen; j++){
    train_feature_data[j] = (train_feature_data[j] - mean) / sd;
  }

  Serial.println(F("----- 農產品識別系統 -----"));
  Serial.println();

  // -------------------------- 建構模型 --------------------------
  // 讀取已訓練的模型檔
  model.begin("/color_model.json");
}

void loop(){
  uint8_t proximity_data = 0;
  uint16_t red_light = 0,green_light = 0,blue_light = 0, ambient_light = 0;

  if(!apds.readProximity(proximity_data)){
    Serial.println("此次讀取接近值錯誤");
  }

  // 當開始蒐集資料的條件達成時, 開始蒐集
  if (proximity_data == 255) {
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LED_ON);

    // 偵測是否要開始蒐集資料
    if(!apds.readAmbientLight(ambient_light)  ||
       !apds.readRedLight(red_light)          ||
       !apds.readGreenLight(green_light)      ||
       !apds.readBlueLight(blue_light)){
      Serial.println("Error reading light values");
    }else{
      // 計算總信號中各種顏色的比例。 它們被標準化為0至1的範圍。
      float sum = red_light + green_light + blue_light;

      // 偵測顏色會用到的參數
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

      // 取得一筆特徵資料, 並使用訓練好的模型來進行預測
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

      // 模型預測
      uint8_t maxIndex = model.argmax(predictVal);
      if(predictVal[maxIndex] > 0.99){
        switch(maxIndex){
          case 0:  Serial.println("草莓"); break;
          case 1:  Serial.println("藍莓"); break;
          case 2:  Serial.println("香蕉"); break;
        }
      }else{
        Serial.println("無法識別");
      }
      
      delay(1000);
    }
  } else {
    // 未蒐集資料時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, LED_OFF);
  }
}
