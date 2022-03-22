/*
  跌倒姿勢記錄 -- 訓練與評估
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_MPU6050.h>

#define LED_ON  0
#define LED_OFF 1

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

// 訓練用的特徵資料
float *train_feature_data;

// 對應特徵資料的標籤值
float *train_label_data;

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

  // 多元分類類型的資料讀取
  data = reader.read("/dataset/others.txt,/dataset/fall.txt", reader.MODE_BINARY);

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

  Serial.println(F("----- 訓練2元分類之跌倒姿勢範例 -----"));
  Serial.println();

  // -------------------------- 建構模型 --------------------------
  Flag_ModelParameter modelPara;
  Flag_LayerSequence nnStructure[] = {{.layerType = model.LAYER_INPUT, .neurons =  0, .activationType = model.ACTIVATION_NONE},     // input layer
                                      {.layerType = model.LAYER_DENSE, .neurons =  8, .activationType = model.ACTIVATION_RELU},     // hidden layer
                                      {.layerType = model.LAYER_DENSE, .neurons =  1, .activationType = model.ACTIVATION_SIGMOID}}; // output layer
  modelPara.inputLayerPara = FLAG_MODEL_2D_INPUT_LAYER_DIM(data->featureDim);
  modelPara.layerSize = FLAG_MODEL_GET_LAYER_SIZE(nnStructure);
  modelPara.layerSeq = nnStructure;
  modelPara.lossFuncType  = model.LOSS_FUNC_MSE;
  modelPara.optimizerPara = {.optimizerType = model.OPTIMIZER_ADAM, .learningRate = 0.001, .epochs = 800, .batch_size = 20}; //optimizer type, learning rate, epochs, batch-size
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
}

void loop() {
  // ----------------------------------------- 評估模型 --------------------------
  // 偵測是否要開始蒐集資料
  if(millis() - lastMeaureTime > 100 && !collect){
    
    // mpu6050資料更新  
    mpu6050.update();
    
    // 開始蒐集資料的條件
    if(mpu6050.data.accY > -0.75){
      Serial.println(mpu6050.data.accY);
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
        Serial.print(F("預測值: "));
        model.printResult(&predictVal);
        
        if(predictVal >= 0.85) Serial.println("評估為已跌倒");
        else                   Serial.println("評估為未跌倒");

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
