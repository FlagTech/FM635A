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

// 訓練用的特徵資料
float *train_feature_data;

// 對應特徵資料的標籤值
float *train_label_data;

// 資料預處理會用到的參數
float mean;
float sd;
float labelMax;
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
  data = reader.read("/dataset/one.txt,/dataset/two.txt,/dataset/three.txt", reader.MODE_CATEGORICAL);

  // 設定訓練用的特徵資料
  train_feature_data = data->feature;

  // 設定對應特徵資料的標籤值
  train_label_data = data->label;

  // 計算特徵資料的平均值
  mean = data->featureMean;

  // 計算特徵資料的標準差
  sd = data->featureSd;
        
  // 特徵資料正規化: 標準差法
  for(int j = 0; j < data->featureDataArryLen; j++){
    train_feature_data[j] = (train_feature_data[j] - mean) / sd;
  }

  // 標籤資料正規化: 分類問題的label不須正規化且標籤資料已由reader做好One-hot encoding了

  Serial.println(F("----- 訓練多元分類之手寫辨識範例 -----"));
  Serial.println(F("請輸入training來開始訓練"));
  Serial.println();
}

void loop() {
  while(Serial.available() > 0 ){
    String str = Serial.readString();
    if(str.indexOf("training") > -1)
    {
      // -------------------------- 建構與訓練模型 --------------------------
      // 創建訓練用的特徵張量
      uint16_t train_feature_shape[] = {data->dataLen, data->featureDim};
      aitensor_t train_feature_tensor = AITENSOR_2D_F32(train_feature_shape, train_feature_data);
  
      // 創建訓練用的標籤張量
      uint16_t train_label_shape[] = {data->dataLen, data->labelDim}; 
      aitensor_t train_label_tensor = AITENSOR_2D_F32(train_label_shape, train_label_data); 

      // 定義神經層
      Flag_LayerSequence nnStructure[] = {{.layerType = model.LAYER_INPUT, .neurons =  0, .activationType = model.ACTIVATION_NONE},     // input layer
                                          {.layerType = model.LAYER_DENSE, .neurons = 10, .activationType = model.ACTIVATION_RELU},     // hidden layer
                                          {.layerType = model.LAYER_DENSE, .neurons =  3, .activationType = model.ACTIVATION_SOFTMAX}}; // output layer
      Flag_ModelTrainParameter modelTrainPara;
      modelTrainPara.feature_tensor  = &train_feature_tensor;
      modelTrainPara.label_tensor = &train_label_tensor;
      modelTrainPara.layerSize = ARRAY_LEN(nnStructure);
      modelTrainPara.layerSeq = nnStructure;
      modelTrainPara.lossFuncType  = model.LOSS_FUNC_CORSS_ENTROPY;
      modelTrainPara.optimizerPara = {.optimizerType = model.OPTIMIZER_ADAM, .learningRate = 0.001, .epochs = 300, .batch_size = 20}; //optimizer type, learning rate, epochs, batch-size
      
      // 訓練模型 
      model.train(&modelTrainPara);
     
      // 匯出模型
      model.save();

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
    }else{
      Serial.println(F("unknown"));
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