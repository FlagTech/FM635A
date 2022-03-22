 /*
  第一個機器學習模型 -- 找出 30-39 歲美國女性的身高與平均體重的關係。
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>

// ------------全域變數------------
// 讀取資料的物件
Flag_DataReader trainDataReader;
Flag_DataReader testDataReader;
Flag_DataBuffer *trainData;
Flag_DataBuffer *testData;

// 神經網路模型
Flag_Model model; 
// -------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // ------------------------- 資料預處理 -------------------------
  // 回歸類型的訓練資料讀取
  trainData = trainDataReader.read(
    "/dataset/women_train.txt", 
    trainDataReader.MODE_REGRESSION
  );
  
  // 取得訓練特徵資料的平均值
  float mean = trainData->featureMean;

  // 取得訓練特徵資料的標準差
  float sd = trainData->featureSd;

  // 訓練特徵資料的正規化: 標準化
  for(int j = 0; j < trainData->featureDataArryLen; j++){
    trainData->feature[j] = (trainData->feature[j] - mean) / sd;
  }

  // 取得標籤資料的最大絕對值
  float labelMaxAbs = trainData->labelMaxAbs;

  // 訓練標籤資料的正規化: 除以標籤最大值
  for(int j = 0; j < trainData->labelDataArryLen; j++){
    trainData->label[j] /= labelMaxAbs;  
  }

  // -------------------------- 建構模型 --------------------------
  Flag_ModelParameter modelPara;
  Flag_LayerSequence nnStructure[] = {
    {  // 輸入層 
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
  modelPara.inputLayerPara = 
    FLAG_MODEL_2D_INPUT_LAYER_DIM(trainData->featureDim);
  modelPara.layerSize = FLAG_MODEL_GET_LAYER_SIZE(nnStructure);                    
  modelPara.layerSeq = nnStructure;
  modelPara.lossFuncType  = model.LOSS_FUNC_MSE;
  modelPara.optimizerPara = {
    .optimizerType = model.OPTIMIZER_ADAM, 
    .learningRate = 0.001, 
    .epochs = 2000, 
    .batch_size = 10
  };
  model.begin(&modelPara);

  // -------------------------- 訓練模型 --------------------------
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
  model.train(&train_feature_tensor, &train_label_tensor);

  // -------------------------- 評估模型 --------------------------
  // 回歸類型的測試資料讀取
  testData = testDataReader.read(
    "/dataset/women_test.txt", 
    testDataReader.MODE_REGRESSION
  );

  Serial.println("預測值:\t\t實際值:");
  for(uint32_t i = 0; i < testData->dataLen; i++){
    uint16_t test_feature_shape[] = {
      1, 
      testData->featureDim
    }; 
    aitensor_t test_feature_tensor = 
      AITENSOR_2D_F32(test_feature_shape, &testData->feature[i]);
    aitensor_t *test_output_tensor;
    float predictVal;
    
    testData->feature[i] = (testData->feature[i] - mean) / sd;

    test_output_tensor = model.predict(&test_feature_tensor); 
    model.getResult(test_output_tensor, labelMaxAbs, &predictVal);  
    Serial.print(predictVal);
    Serial.print("\t\t");
    Serial.println(testData->label[i]); 
  }
}

void loop() {}
