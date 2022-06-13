/*
  水果熟成分類 -- 訓練與評估
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>

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
//--------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // 初始化 APDS9960
  while(!apds.init()){
    Serial.println("APDS-9960 初始化錯誤");
  }

  // 啟用 APDS-9960 光感測器
  while(!apds.enableLightSensor()){
    Serial.println("光感測器初始化錯誤");
  }

  // 啟用 APDS-9960 接近感測器
  while(!apds.enableProximitySensor()){
    Serial.println("接近感測器初始化錯誤");
  }
  
  // 腳位設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // ----------------- 資料預處理 ------------------
  // 二元分類類型的訓練資料讀取
  trainData = trainDataReader.read(
    "/dataset/unripe.txt,/dataset/ripe.txt", 
    trainDataReader.MODE_BINARY
  );
 
  // 取得訓練特徵資料的平均值
  mean = trainData->featureMean;

  // 取得訓練特徵資料的標準差
  sd = trainData->featureSd;
        
  // 縮放訓練特徵資料: 標準化
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
    { // 輸出層  
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
    .epochs = 3000
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

void loop(){
  // ----------------- 評估模型 --------------------
  float sensorData[trainData->featureDim]; 
  uint8_t proximity_data = 0;
  uint16_t red_light = 0;
  uint16_t green_light = 0;
  uint16_t blue_light = 0;
  uint16_t ambient_light = 0;

  if(!apds.readProximity(proximity_data)){
    Serial.println("讀取接近值錯誤");
  }

  // 當足夠接近時, 開始蒐集
  if (proximity_data == 255) {
    // 辨識時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LOW);

    if(!apds.readAmbientLight(ambient_light)  ||
       !apds.readRedLight(red_light)          ||
       !apds.readGreenLight(green_light)      ||
       !apds.readBlueLight(blue_light)){
      Serial.println("讀值錯誤");
    }else{
      // 取得各種顏色的比例
      float sum = red_light + green_light + blue_light;
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
      aitensor_t test_feature_tensor = AITENSOR_2D_F32(
        test_feature_shape, 
        test_feature_data
      );
      aitensor_t *test_output_tensor;

      test_output_tensor = model.predict(
        &test_feature_tensor
      );
      
      // 輸出預測結果
      float predictVal;
      model.getResult(
        test_output_tensor, 
        &predictVal
      );
      Serial.print("預測結果: ");
      Serial.print(predictVal);
      if(predictVal > 0.5) Serial.println(" 已熟成");
      else                 Serial.println(" 未熟成");
      delay(1000);
    }
  } else {
    // 未辨識時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
