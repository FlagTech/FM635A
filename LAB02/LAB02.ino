/*
  第一個機器學習模型 -- 找出 30-39 歲美國女性的身高與平均體重的關係。
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>

// ------------全域變數------------
// 讀取資料的物件
Flag_DataReader trainDataReader;
Flag_DataReader testDataReader;

// 指向存放資料的指位器
Flag_DataBuffer *trainData;
Flag_DataBuffer *testData;

// 神經網路模型
Flag_Model model; 
// -------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // ------------------------- 資料預處理 -------------------------
  // 迴歸類型的訓練資料讀取
  trainData = trainDataReader.read(
    "/dataset/women_train.txt", 
    trainDataReader.MODE_REGRESSION
  );
  
  // 取得訓練特徵資料的平均值
  float mean = trainData->featureMean;

  // 取得訓練特徵資料的標準差
  float sd = trainData->featureSd;

  // 縮放訓練特徵資料: 標準化
  for(int j = 0; j < trainData->featureDataArryLen; j++){
    trainData->feature[j] = (trainData->feature[j] - mean) / sd;
  }

  // 取得最大訓練標籤資料的絕對值
  float labelMaxAbs = trainData->labelMaxAbs;

  // 縮放訓練標籤資料: 除以最大標籤的絕對值
  for(int j = 0; j < trainData->labelDataArryLen; j++){
    trainData->label[j] /= labelMaxAbs;  
  }

  // -------------------------- 建構模型 --------------------------
  Flag_ModelParameter modelPara;
  Flag_LayerSequence nnStructure[] = {
    { // 輸入層 
      .layerType = model.LAYER_INPUT, 
      .neurons =  0, 
      .activationType = model.ACTIVATION_NONE
    },  
    { // 第 1 層隱藏層 
      .layerType = model.LAYER_DENSE, 
      .neurons =  5, 
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
  modelPara.layerSize = FLAG_MODEL_GET_LAYER_SIZE(nnStructure);
  modelPara.inputLayerPara = 
    FLAG_MODEL_2D_INPUT_LAYER_DIM(trainData->featureDim);
  modelPara.lossFuncType  = model.LOSS_FUNC_MSE;
  modelPara.optimizerPara = {
    .optimizerType = model.OPTIMIZER_ADAM, 
    .learningRate = 0.001, 
    .epochs = 2000
  };
  model.begin(&modelPara);

  // -------------------------- 訓練模型 --------------------------
  // 創建訓練用的特徵張量
  uint16_t train_feature_shape[] = {
    trainData->dataLen,    // 特徵資料筆數
    trainData->featureDim  // 特徵資料維度
  };

  aitensor_t train_feature_tensor = AITENSOR_2D_F32(
    train_feature_shape,   // 特徵資料形狀
    trainData->feature     // 特徵資料來源
  );

  // 創建訓練用的標籤張量
  uint16_t train_label_shape[] = {
    trainData->dataLen,    // 標籤資料筆數
    trainData->labelDim    // 標籤資料維度
  }; 

  aitensor_t train_label_tensor = AITENSOR_2D_F32(
    train_label_shape,     // 標籤資料形狀
    trainData->label       // 標籤資料來源
  ); 

  // 訓練模型 
  model.train(&train_feature_tensor, &train_label_tensor);

  // -------------------------- 評估模型 --------------------------
  // 迴歸類型的測試資料讀取
  testData = testDataReader.read(
    "/dataset/women_test.txt", 
    testDataReader.MODE_REGRESSION
  );

  Serial.println("預測值:\t\t實際值:");
  for(uint32_t i = 0; i < testData->dataLen; i++){
    // 測試資料預處理
    testData->feature[i] = (testData->feature[i] - mean) / sd;

    // 模型預測
    uint16_t test_feature_shape[] = {
      1, // 每次測試一筆資料
      testData->featureDim
    }; 
    aitensor_t test_feature_tensor = 
      AITENSOR_2D_F32(test_feature_shape, &testData->feature[i]);
    aitensor_t *test_output_tensor;
    
    test_output_tensor = model.predict(&test_feature_tensor);

    // 輸出預測結果
    float predictVal;
    model.getResult(test_output_tensor, labelMaxAbs, &predictVal);
    Serial.print(predictVal);
    Serial.print("\t\t");
    Serial.println(testData->label[i]); 
  }
}

void loop() {}
