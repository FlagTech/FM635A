#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_MPU6050.h>
#include <Flag_Switch.h>

#define LED_ON  0
#define LED_OFF 1
#define BTN_PIN 12
#define CLASS_TOTAL 3 
#define PERIOD 10
#define FEATURE_PARA 6
#define FEATURE_DIM PERIOD * FEATURE_PARA

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

  // 讀取已訓練的模型檔
  model.debugInfoTypeConfig(model.INFO_VERBOSE);
  model.config("/gesture_model.json");

  // 多元分類類型的資料讀取
  reader.config("/dataset/one.txt,/dataset/two.txt,/dataset/three.txt", reader.MODE_CATEGORICAL);  //注意讀檔案順序分別對應到one-hot encoding
  data = reader.read();

  // 計算特徵資料的平均值
  mean = data->featureMean;

  // 計算特徵資料的標準差
  sd = data->featureSd;
        
  Serial.println(F("----- 即時預測手寫辨識 -----"));
  Serial.println();
}

void loop() {
  // ----------------------------------------- 評估模型 --------------------------
  float test_data[FEATURE_DIM];
  uint32_t testArrayIndex = 0;
  uint32_t finishedCond = 0;
  uint32_t lastMeaureTime = 0;
  
  while(1){
    if(btn.read()){
      digitalWrite(LED_BUILTIN, LED_ON);
    }else{
      if(!btn.read()) {
        digitalWrite(LED_BUILTIN, LED_OFF);
        finishedCond = 0;
        testArrayIndex = 0;
      }
    }
      
    if(btn.read()){
      if(millis() - lastMeaureTime > 100){
        mpu6050.update();
        if(finishedCond == PERIOD){
          //測試資料預處理
          for(int i = 0; i <ARRAY_LEN(test_data) ; i++){
            test_data[i] -= mean;
            test_data[i] /= sd;
          }

          // 使用訓練好的模型來預測
          float *test_feature_data = test_data; 
          uint16_t test_feature_shape[] = {1, sizeof(test_data) / sizeof(test_data[0])};
          aitensor_t test_feature_tensor = AITENSOR_2D_F32(test_feature_shape, test_feature_data);
          aitensor_t *test_output_tensor;
          
          test_output_tensor = model.predict(&test_feature_tensor);

          float predictVal[CLASS_TOTAL];
          for(int i = 0; i<CLASS_TOTAL; i++){
            predictVal[i] = ((float* ) test_output_tensor->data)[i]; 
          }
          
          // Check result
          Serial.print(F("Calculated output: "));
          for(int i = 0; i<CLASS_TOTAL-1; i++){
            Serial.print(predictVal[i]); Serial.print(", ");  
          }
          Serial.println(predictVal[CLASS_TOTAL-1]);
          Serial.println(F("")); 

          // find index of max value
          float max = predictVal[0];
          uint8_t max_index = 0;
          for(int i = 1; i<CLASS_TOTAL; i++){
            if(predictVal[i]>max){
              max = predictVal[i];
              max_index = i;
            }
          }
          Serial.print("你寫的數字為: ");
          Serial.println(max_index + 1);
          while(btn.read());
          
        }else{
          test_data[testArrayIndex] = mpu6050.data.accX; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.accY; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.accZ; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.gyrX; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.gyrY; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.gyrZ; testArrayIndex++;
          finishedCond++;
        }
        lastMeaureTime = millis();
      }
    }
  }
}


// // 確認讀取到的資料
// for(int j = 0; j < data->dataLen; j++){
//   Serial.print(F("Feature Data :"));
//   for(int i = 0; i < data->featureDim; i++){
//     Serial.print(data->feature[i + j * data->featureDim]);
//     if(i != data->featureDim - 1){
//       Serial.print(F(","));
//     }
//   }
//   Serial.print(F("\t\t"));

//   Serial.print("Label Data :");
//   for(int i = 0; i < data->labelDim; i++){
//     Serial.print(data->label[i +  j * data->labelDim]);
//     if(i != data->labelDim - 1){
//       Serial.print(F(","));
//     }
//   }
//   Serial.println(F(""));
// }