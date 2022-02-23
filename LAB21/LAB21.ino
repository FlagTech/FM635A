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

// 訓練用的特徵資料
float *train_feature_data;

// 對應特徵資料的標籤值
float *train_label_data;

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
  data = reader.read("/dataset/one.txt,/dataset/two.txt,/dataset/three.txt", reader.MODE_CATEGORICAL);

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

  // 標籤資料正規化: 分類問題的label不須正規化且標籤資料已由reader做好One-hot encoding了

  Serial.println(F("----- 訓練多元分類之手寫辨識範例 -----"));
  Serial.println();
}

void loop() {
  // -------------------------- 建構模型 --------------------------
  uint32_t classNum = reader.getNumOfFiles();
  Flag_ModelParameter modelPara;
  Flag_LayerSequence nnStructure[] = {{.layerType = model.LAYER_INPUT, .neurons =  0,       .activationType = model.ACTIVATION_NONE},     // input layer
                                      {.layerType = model.LAYER_DENSE, .neurons = 10,       .activationType = model.ACTIVATION_RELU},     // hidden layer
                                      {.layerType = model.LAYER_DENSE, .neurons = classNum, .activationType = model.ACTIVATION_SOFTMAX}}; // output layer
  modelPara.inputLayerPara = FLAG_MODEL_2D_INPUT_LAYER_DIM(data->featureDim);
  modelPara.layerSize = FLAG_MODEL_GET_LAYER_SIZE(nnStructure);
  modelPara.layerSeq = nnStructure;
  modelPara.lossFuncType  = model.LOSS_FUNC_CORSS_ENTROPY;
  modelPara.optimizerPara = {.optimizerType = model.OPTIMIZER_ADAM, .learningRate = 0.001, .epochs = 300, .batch_size = 20};
  model.begin(&modelPara);

  // -------------------------- 訓練模型 --------------------------
  // 創建訓練用的特徵張量
  uint16_t train_feature_shape[] = {data->dataLen, data->featureDim};
  aitensor_t train_feature_tensor = AITENSOR_2D_F32(train_feature_shape, train_feature_data);

  // 創建訓練用的標籤張量
  uint16_t train_label_shape[] = {data->dataLen, data->labelDim}; 
  aitensor_t train_label_tensor = AITENSOR_2D_F32(train_label_shape, train_label_data); 

  // 訓練模型 
  model.train(&train_feature_tensor, &train_label_tensor);
  
  // 匯出模型
  model.save();

  // -------------------------- 評估模型 --------------------------
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
          float *eval_feature_data = sensor_data; 
          uint16_t eval_feature_shape[] = {1, FEATURE_DIM};
          aitensor_t eval_feature_tensor = AITENSOR_2D_F32(eval_feature_shape, eval_feature_data);
          aitensor_t *eval_output_tensor;
          float predictVal[model.getNumOfOutputs()];

          // 測試資料預處理
          for(int i = 0; i < FEATURE_DIM ; i++){
            eval_feature_data[i] = (eval_feature_data[i] - mean) / sd;
          }

          // 模型預測
          eval_output_tensor = model.predict(&eval_feature_tensor);
          model.getResult(eval_output_tensor, predictVal);
          
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