/*
  電子料理秤 -- 訓練與評估
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_HX711.h>

// ------------全域變數------------
// 讀取資料的物件
Flag_DataReader reader;
Flag_DataBuffer *data;

// 神經網路模型
Flag_Model model; 

// 感測器的物件
Flag_HX711 hx711(16, 17); 

// 訓練用的特徵資料
float *train_feature_data;

// 對應特徵資料的標籤值
float *train_label_data;

// 資料預處理會用到的參數
float mean;
float sd;
float labelMaxAbs;
// -------------------------------

void setup() {
  // UART設置
  Serial.begin(115200);

  // hx711設置
  hx711.tare();  // 歸零調整, 取得offset平均值
  Serial.print("offset : ");
  Serial.println(hx711.getOffset());

  // 回歸類型的資料讀取
  data = reader.read("/dataset/weight.txt", reader.MODE_REGRESSION);

  // 設定訓練用的特徵資料
  train_feature_data = data->feature;

  // 設定對應特徵資料的標籤值
  train_label_data = data->label;

  // 取得特徵資料的平均值
  mean = data->featureMean;

  // 取得特徵資料的標準差
  sd = data->featureSd;

  // 取得標籤資料的最大絕對值
  labelMaxAbs = data->labelMaxAbs; 

  // 特徵資料正規化: 標準差法
  for(int j = 0; j < data->featureDataArryLen; j++){
    train_feature_data[j] = (train_feature_data[j] - mean) / sd;
  }

  // 標籤資料正規化: Min/Max法
  for(int j = 0; j < data->labelDataArryLen; j++){
    train_label_data[j] /= labelMaxAbs;  
  }

  Serial.println(F("----- 訓練<HX711 - 重量>特性圖範例 -----"));
  Serial.println();

  // -------------------------- 建構模型 --------------------------
  Flag_ModelParameter modelPara;
  Flag_LayerSequence nnStructure[] = {{.layerType = model.LAYER_INPUT, .neurons =  0, .activationType = model.ACTIVATION_NONE},  // input layer
                                      {.layerType = model.LAYER_DENSE, .neurons = 10, .activationType = model.ACTIVATION_RELU},  // hidden layer
                                      {.layerType = model.LAYER_DENSE, .neurons = 10, .activationType = model.ACTIVATION_RELU},  // hidden layer
                                      {.layerType = model.LAYER_DENSE, .neurons =  1, .activationType = model.ACTIVATION_RELU}}; // output layer          
  modelPara.inputLayerPara = FLAG_MODEL_2D_INPUT_LAYER_DIM(data->featureDim);
  modelPara.layerSize = FLAG_MODEL_GET_LAYER_SIZE(nnStructure);                    
  modelPara.layerSeq = nnStructure;
  modelPara.lossFuncType  = model.LOSS_FUNC_MSE;
  modelPara.optimizerPara = {.optimizerType = model.OPTIMIZER_ADAM, .learningRate = 0.001, .epochs = 1000, .batch_size = 10};
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
  // -------------------------- 評估模型 --------------------------
  // 使用訓練好的模型來預測
  float eval_feature_data;  
  uint16_t eval_feature_shape[] = {1, data->featureDim}; 
  aitensor_t eval_feature_tensor = AITENSOR_2D_F32(eval_feature_shape, &eval_feature_data);
  aitensor_t *eval_output_tensor;
  float predictVal;
  
  Serial.println("量測值:\t\t預測值:");
  while(1){
    // 測試資料預處理
    eval_feature_data = (hx711.getWeight() - mean) / sd;

    // 模型預測
    eval_output_tensor = model.predict(&eval_feature_tensor); 
    
    // 輸出預測結果
    Serial.print(hx711.getWeight());
    Serial.print("\t\t");
    model.getResult(eval_output_tensor, labelMaxAbs, &predictVal);  //因為標籤資料有經過正規化, 所以要將label的比例還原回來
    Serial.println(predictVal);
  }
}