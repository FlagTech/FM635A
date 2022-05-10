/*
  顏色辨識感測器 -- 訓練與評估
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_DataExporter.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>

// 取 APDS9960 的 3 個參數為一筆特徵資料
#define FEATURE_DIM 3

//------------全域變數------------
// 讀取資料的物件
Flag_DataReader trainDataReader;

// 指向存放資料的指位器
Flag_DataBuffer *trainData;

// 神經網路模型
Flag_Model model; 

// 感測器的物件
SparkFun_APDS9960 apds = SparkFun_APDS9960();

// 資料預處理會用到的參數
float mean;
float sd;

// 蒐集資料會用到的參數
float sensorData[FEATURE_DIM]; 
//--------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // 初始化 APDS9960
  if(apds.init()){
    Serial.println("APDS-9960 初始化完成");
  }else{
    Serial.println("APDS-9960 初始化錯誤");
  }

  // 啟用 APDS-9960 光感測器
  if(apds.enableLightSensor(false)){
    Serial.println("光感測器正在運行");
  }else{
    Serial.println("光感測器初始化錯誤");
  }

  // 啟用 APDS-9960 接近感測器
  if(apds.enableProximitySensor(false)){
    Serial.println("接近感測器正在運行");
  }else{ 
    Serial.println("接近感測器初始化錯誤");
  }
  
  // 腳位設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // ----------------- 資料預處理 ------------------
  // 多元分類類型的資料讀取
  trainData = trainDataReader.read(
    "/dataset/unripe.txt,/dataset/ripe.txt", 
    trainDataReader.MODE_BINARY
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
  Flag_ModelParameter modelPara;
  Flag_LayerSequence nnStructure[] = {
    { // 輸入層
      .layerType = model.LAYER_INPUT,
      .neurons =  0,
      .activationType = model.ACTIVATION_NONE
    },
    { // 第 1 層隱藏層
      .layerType = model.LAYER_DENSE, 
      .neurons = 5, 
      .activationType = model.ACTIVATION_RELU
    },
    { // 第 2 層隱藏層 
      .layerType = model.LAYER_DENSE, 
      .neurons = 10, 
      .activationType = model.ACTIVATION_RELU
    },
    {
      .layerType = model.LAYER_DENSE,
      .neurons = 1,
      .activationType = model.ACTIVATION_SIGMOID
    }
  };
  modelPara.layerSeq = nnStructure;
  modelPara.layerSize = 
    FLAG_MODEL_GET_LAYER_SIZE(nnStructure);
  modelPara.inputLayerPara = 
    FLAG_MODEL_2D_INPUT_LAYER_DIM(
      trainData->featureDim
    );
  modelPara.lossFuncType  = model.LOSS_FUNC_MSE;
  modelPara.optimizerPara = {
    .optimizerType = model.OPTIMIZER_ADAM,
    .learningRate = 0.001,
    .epochs = 2000
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
  model.save();
}

void loop(){
  uint8_t proximity_data = 0;
  uint16_t red_light = 0;
  uint16_t green_light = 0;
  uint16_t blue_light = 0;
  uint16_t ambient_light = 0;

  if(!apds.readProximity(proximity_data)){
    Serial.println("此次讀取接近值錯誤");
  }

  // 當開始蒐集資料的條件達成時, 開始蒐集
  if (proximity_data == 255) {
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LOW);

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

      // 取得一筆特徵資料, 並使用訓練好的模型來預測以進行評估
      float *eval_feature_data = sensorData; 
      uint16_t eval_feature_shape[] = {1, FEATURE_DIM};
      aitensor_t eval_feature_tensor = AITENSOR_2D_F32(
        eval_feature_shape, 
        eval_feature_data
      );
      aitensor_t *eval_output_tensor;
      float predictVal;

      // 測試資料預處理
      for(int i = 0; i < FEATURE_DIM ; i++){
        eval_feature_data[i] = 
          (eval_feature_data[i] - mean) / sd;
      }

      // 模型預測
      eval_output_tensor = model.predict(&eval_feature_tensor);
      model.getResult(eval_output_tensor, &predictVal);
      
      // 輸出預測結果
      Serial.print("預測結果: ");

      // 找到機率最大的索引值
      Serial.println(predictVal);
      
      delay(1000);
    }
  } else {
    // 未蒐集資料時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
