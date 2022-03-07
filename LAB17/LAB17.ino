/*
  跌倒偵測警示器
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_MPU6050.h>

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

// 資料預處理會用到的參數
float mean;
float sd;

// 即時預測會用到的參數
float sensorData[FEATURE_DIM];
uint32_t sensorArrayIndex = 0;
uint32_t collectFinishedCond = 0;
uint32_t lastMeaureTime = 0;
bool collect = false;
//--------------------------------

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

    // 開始蒐集資料的條件
    if(mpu6050.data.accY > -0.75){
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
        float *test_feature_data = sensorData; 
        uint16_t test_feature_shape[] = {1, FEATURE_DIM};
        aitensor_t test_feature_tensor = AITENSOR_2D_F32(test_feature_shape, test_feature_data);
        aitensor_t *test_output_tensor;
        float predictVal;

        // 測試資料預處理
        for(int i = 0; i < FEATURE_DIM ; i++){
          test_feature_data[i] = (test_feature_data[i] - mean) / sd;
        }

        // 模型預測
        test_output_tensor = model.predict(&test_feature_tensor);
        model.getResult(test_output_tensor, &predictVal);
        
        // 輸出預測結果
        if(predictVal >= 0.85) {
          Serial.println("已跌倒");

          // 發出警示聲
          for(int i = 0; i < 5; i++){
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
            digitalWrite(BUZZER_PIN, LOW);
            delay(500);
          }
        }else{
          Serial.println("未跌倒");
        }

        // 下次蒐集特徵資料時, 要重新蒐集
        sensorArrayIndex = 0;
        collectFinishedCond = 0;
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
  }
}
