/*
  電子秤 -- 訓練與評估
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_HX711.h>

// ------------全域變數------------
// 讀取資料的物件
Flag_DataReader trainDataReader;

// 指向存放資料的指位器
Flag_DataBuffer *trainData;

// 神經網路模型
Flag_Model model; 

// 感測器的物件
Flag_HX711 hx711(32, 33);

// 資料預處理會用到的參數
float mean;
float sd;
float labelMaxAbs;
// -------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // HX711 初始化
  hx711.begin();

  // 扣重調整
  hx711.tare();  

  // ----------------- 資料預處理 ------------------
  // 迴歸類型的訓練資料讀取
  trainData = trainDataReader.read(
    "/dataset/weight.txt", 
    trainDataReader.MODE_REGRESSION
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

  // 取得最大訓練標籤資料的絕對值
  labelMaxAbs = trainData->labelMaxAbs; 

  // 縮放訓練標籤資料: 除以最大標籤的絕對值
  for(int j = 0;
      j < trainData->labelDataArryLen;
      j++)
  {
    trainData->label[j] /= labelMaxAbs;  
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
      .neurons =  1, 
      .activationType = model.ACTIVATION_RELU
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
  model.save(mean, sd, labelMaxAbs);
}

void loop() {
  // ----------------- 評估模型 --------------------
  // 測試資料預處理
  float test_feature_data = 
    (hx711.getWeight() - mean) / sd;

  // 模型預測
  uint16_t test_feature_shape[] = {
    1, // 每次測試一筆資料
    trainData->featureDim
  }; 
  aitensor_t test_feature_tensor = AITENSOR_2D_F32(
    test_feature_shape, 
    &test_feature_data
  );
  aitensor_t *test_output_tensor;
  
  test_output_tensor = model.predict(
    &test_feature_tensor
  );
  
  // 輸出預測結果
  float predictVal;
  model.getResult(
    test_output_tensor,
    labelMaxAbs, 
    &predictVal
  );
  Serial.print("預測值: ");
  Serial.print(predictVal, 1);
  Serial.println("g");
}
