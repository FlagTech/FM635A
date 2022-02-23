#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_MPU6050.h>
#include <Flag_Switch.h>

#define LED_ON  0
#define LED_OFF 1
#define BTN_PIN 12

//1 period取6個para, 每10 period為一筆feature data;
#define PERIOD 10
#define SENSOR_PARA 6
#define FEATURE_DIM (PERIOD * SENSOR_PARA)

//------------全域變數------------
Flag_DataReader reader;
Flag_DataBuffer *data;
Flag_Model model; 
Flag_MPU6050 mpu6050;
Flag_Switch btn(BTN_PIN, INPUT);

// 資料預處理會用到的參數
float mean;
float sd;
//--------------------------------

void setup() {
  // UART設置
  Serial.begin(115200);

  // mpu6050設置
  mpu6050.init();
  while(!mpu6050.isReady()); 

  // GPIO設置
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BTN_PIN, INPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  // 多元分類類型的資料讀取
  data = reader.read("/dataset/one.txt,/dataset/two.txt,/dataset/three.txt", reader.MODE_CATEGORICAL); //注意讀檔案順序分別對應到one-hot encoding

  // 取得特徵資料的平均值
  mean = data->featureMean;

  // 取得特徵資料的標準差
  sd = data->featureSd;
        
  Serial.println(F("----- 即時預測手寫辨識 -----"));
  Serial.println();
}

void loop() {
  // -------------------------- 建構模型 --------------------------
  // 讀取已訓練的模型檔
  model.begin("/gesture_model.json");

  // -------------------------- 即時預測 --------------------------
  float sensor_data[FEATURE_DIM];
  uint32_t sensorArrayIndex = 0;
  uint32_t collectFinishedCond = 0;
  uint32_t lastMeaureTime = 0;
  
  while(1){
    // 當按鈕按下時開始收集資料   
    if(btn.read()){
      // 收集資料時, 內建指示燈會亮
      digitalWrite(LED_BUILTIN, LED_ON);

      //100ms取一次, 共取10次, 也就是一秒
      if(millis() - lastMeaureTime > 100){
        //mpu6050資料更新  
        mpu6050.update();

        if(collectFinishedCond == PERIOD){
          // 使用訓練好的模型來預測
          float *test_feature_data = sensor_data; 
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
          
          // 輸出預測結果
          Serial.print(F("Calculated output: "));
          model.printResult(predictVal);

          // 找到機率最大的索引值
          uint8_t maxIndex = model.argmax(predictVal);
          Serial.print("你寫的數字為: ");
          Serial.println(maxIndex + 1);

          // 當還按著按鈕則阻塞, 直到放開按鈕
          while(btn.read());
          
        }else{
          sensor_data[sensorArrayIndex] = mpu6050.data.accX; sensorArrayIndex++;
          sensor_data[sensorArrayIndex] = mpu6050.data.accY; sensorArrayIndex++;
          sensor_data[sensorArrayIndex] = mpu6050.data.accZ; sensorArrayIndex++;
          sensor_data[sensorArrayIndex] = mpu6050.data.gyrX; sensorArrayIndex++;
          sensor_data[sensorArrayIndex] = mpu6050.data.gyrY; sensorArrayIndex++;
          sensor_data[sensorArrayIndex] = mpu6050.data.gyrZ; sensorArrayIndex++;
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
  }
}
