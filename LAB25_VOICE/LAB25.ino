/*
  跌倒偵測警示器
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_MPU6050.h>

#define LED_ON  0
#define LED_OFF 1

// 1個週期(PERIOD)取MPU6050的6個參數(SENSOR_PARA)
// 每10個週期(PERIOD)為一筆特徵資料
#define PERIOD 200
#define SENSOR_PARA 1
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

// 評估模型會用到的參數
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
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  analogSetAttenuation(ADC_11db); // 衰減11db, 目的是可以量測到3.3v的輸入電壓
  analogSetWidth(12);           // 12位元解析度

  // 2元分類類型的資料讀取
  data = reader.read("/dataset/off.txt,/dataset/on.txt", reader.MODE_BINARY); //注意讀檔案順序分別對應到one-hot encoding

  // 取得特徵資料的平均值
  mean = data->featureMean;

  // 取得特徵資料的標準差
  sd = data->featureSd;
        
  Serial.println(F("----- 即時語音辨識 -----"));
  Serial.println();

  // -------------------------- 建構模型 --------------------------
  // 讀取已訓練的模型檔
  model.debugInfoTypeConfig(model.INFO_VERBOSE);
  model.begin("/voice_model.json");
}

void loop() {
  // -------------------------- 即時預測 --------------------------
  // 偵測是否要開始蒐集資料
  if(millis() - lastMeaureTime > 1 && !collect){
    // mpu6050資料更新  
    if(analogRead(A4) > 1500) collect = true;
    
    lastMeaureTime = millis();
  }

  // 當按鈕按下時開始蒐集資料   
  if(collect){
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LED_ON);
  
    // 100ms為一個週期來取一次mpu6050資料, 連續取10個週期作為一筆特徵資料, 也就是一秒會取到一筆特徵資料
    if(millis() - lastMeaureTime > 1){
      // mpu6050資料更新  
      uint16_t adc = analogRead(A4);
      Serial.println(adc);
      if(collectFinishedCond == PERIOD){
         // 使用訓練好的模型來預測
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

        collect = false;

        
      }else{
        sensorData[sensorArrayIndex] = (float)adc; sensorArrayIndex++;
        collectFinishedCond++;
      }
      lastMeaureTime = millis();
    }
  }else{
    // 未蒐集資料時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, LED_OFF);
    
    // 按鈕放開, 則代表特徵資料要重新蒐集
    collectFinishedCond = 0;
    sensorArrayIndex = 0;


  }                 
}
