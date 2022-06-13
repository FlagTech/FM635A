/*
  手勢紀錄 -- 訓練與評估
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_MPU6050.h>
#include <Flag_Switch.h>

// 1 個週期取 MPU6050 的 6 個參數
// 每 10 個週期為一筆特徵資料
#define SENSOR_PARA 6
#define PERIOD 10

//------------全域變數------------
// 讀取資料的物件
Flag_DataReader trainDataReader;

// 指向存放資料的指位器
Flag_DataBuffer *trainData;

// 神經網路模型
Flag_Model model; 

// 感測器的物件
Flag_MPU6050 mpu6050;
Flag_Switch collectBtn(19);

// 資料預處理會用到的參數
float mean;
float sd;

// 評估模型會用到的參數
float sensorData[PERIOD * SENSOR_PARA];
uint32_t sensorArrayIdx;
uint32_t collectFinishedCond;
uint32_t lastMeasTime;
//--------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // MPU6050 初始化
  mpu6050.init();
  while(!mpu6050.isReady()); 

  // 腳位設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // 清除蒐集資料會用到的參數
  sensorArrayIdx = 0;
  collectFinishedCond = 0;
  lastMeasTime = 0;

  // ----------------- 資料預處理 ------------------
  // 多元分類類型的資料讀取
  trainData = trainDataReader.read(
"/dataset/one.txt,/dataset/two.txt,/dataset/three.txt",
    trainDataReader.MODE_CATEGORICAL
  );
  
  // 取得特徵資料的平均值
  mean = trainData->featureMean;

  // 取得特徵資料的標準差
  sd = trainData->featureSd;
        
  // 特徵資料正規化: 標準差法
  for(int j = 0; 
      j < trainData->featureDataArryLen;
      j++)
  {
    trainData->feature[j] = 
      (trainData->feature[j] - mean) / sd;
  }

  // ----------------- 建構模型 --------------------
  uint32_t classNum = trainDataReader.getNumOfFiles();
  Flag_ModelParameter modelPara;
  Flag_LayerSequence nnStructure[] = {
    { // 輸入層
      .layerType = model.LAYER_INPUT,
      .neurons =  0,
      .activationType = model.ACTIVATION_NONE
    },
    { // 第 1 層隱藏層 
      .layerType = model.LAYER_DENSE, 
      .neurons = 10, 
      .activationType = model.ACTIVATION_RELU
    },
    { // 輸出層  
      .layerType = model.LAYER_DENSE,
      .neurons = classNum,
      .activationType = model.ACTIVATION_SOFTMAX
    }
  };
  modelPara.layerSeq = nnStructure;
  modelPara.layerSize = 
    FLAG_MODEL_GET_LAYER_SIZE(nnStructure);
  modelPara.inputLayerPara = 
    FLAG_MODEL_2D_INPUT_LAYER_DIM(
      trainData->featureDim
    );
  modelPara.lossFuncType = 
    model.LOSS_FUNC_CORSS_ENTROPY;
  modelPara.optimizerPara = {
    .optimizerType = model.OPTIMIZER_ADAM,
    .learningRate = 0.001,
    .epochs = 1000,
  };
  model.begin(&modelPara);

  // ----------------- 訓練模型 --------------------
  // 創建訓練用的特徵張量
  uint16_t train_feature_shape[] = {
    trainData->dataLen,
    trainData->featureDim
  };
  aitensor_t train_feature_tensor = AITENSOR_2D_F32(
    train_feature_shape, 
    trainData->feature
  );

  // 創建訓練用的標籤張量
  uint16_t train_label_shape[] = {
    trainData->dataLen,
    trainData->labelDim
  }; 
  aitensor_t train_label_tensor = AITENSOR_2D_F32(
    train_label_shape, 
    trainData->label
  ); 

  // 訓練模型 
  model.train(
    &train_feature_tensor,
    &train_label_tensor
  );
  
  // 匯出模型
  model.save(mean, sd);
}

void loop() {
  // ----------------- 評估模型 --------------------
  // 當按鈕按下時, 開始蒐集資料
  if(collectBtn.read()){
    // 蒐集資料時, 點亮內建指示燈
    digitalWrite(LED_BUILTIN, LOW);

    // 每 100 毫秒為一個週期來取一次 MPU6050 資料
    if(millis() - lastMeasTime > 100){
      // MPU6050 資料更新
      mpu6050.update();
      sensorData[sensorArrayIdx] = mpu6050.data.accX;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.accY;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.accZ;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.gyrX;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.gyrY;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.gyrZ;
      sensorArrayIdx++;
      collectFinishedCond++;
      
      // 連續取 10 個週期作為一筆特徵資料, 
      // 也就是一秒會取到一筆特徵資料
      if(collectFinishedCond == PERIOD){
        // 測試資料預處理
        float *test_feature_data = sensorData;
        for(int i = 0; i < trainData->featureDim; i++){
          test_feature_data[i] = 
            (sensorData[i] - mean) / sd;
        }
        
        // 模型預測
        uint16_t test_feature_shape[] = {
          1, // 每次測試一筆資料
          trainData->featureDim
        };
        aitensor_t test_feature_tensor=AITENSOR_2D_F32(
          test_feature_shape,
          test_feature_data
        );
        aitensor_t *test_output_tensor;
        test_output_tensor = model.predict(
          &test_feature_tensor
        );

        // 輸出預測結果
        float predictVal[model.getNumOfOutputs()];
        model.getResult(
          test_output_tensor,
          predictVal
        );
        Serial.print("預測結果: ");
        model.printResult(predictVal);
        
        // 找到信心值最大的索引
        uint8_t maxIndex = model.argmax(predictVal);
        Serial.print("你寫的數字為: "); 
        Serial.println(maxIndex + 1);

        // 要先放開按鈕才能再做資料的蒐集
        while(collectBtn.read());
      }
      lastMeasTime = millis();
    }
  }else{
    // 未蒐集資料時, 熄滅內建指示燈
    digitalWrite(LED_BUILTIN, HIGH);

    // 按鈕放開, 則代表特徵資料要重新蒐集
    collectFinishedCond = 0;

    // 若中途手放開按鈕, 則不足以形成一筆特徵資料
    sensorArrayIdx = 0;
  }
}
