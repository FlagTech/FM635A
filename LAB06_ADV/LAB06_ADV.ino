 /*
  電子料理秤 -- 訓練與評估
  --------------------
  該草圖展現如何在AIfES中使用訓練數據來訓練神經網絡。
  網絡結構為1-10(relu))-10(relu)-1(relu)，使用relu作為激活函數。
  使用Glorot初始化。
  使用ADAM優化器的訓練。
  優化器執行100個 epoch 的批量訓練。
  計算在 float 32 中完成。
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_TaskManager.h>

// ------------全域變數------------
Flag_DataReader reader;
Flag_DataBuffer *data;
Flag_Model model; 
Flag_TaskManager taskManager;

// 訓練用的特徵資料
float *train_feature_data;

// 對應特徵資料的標籤值
float *train_label_data;

// 資料預處理會用到的參數
float mean;
float sd;
float labelMaxAbs;
// -------------------------------

// 建構模型
Flag_Task createModel = {
  .active = true, 
  .period = taskManager.ONE_SHOT, 
  .callback = [](){
    Flag_ModelParameter modelPara;
    Flag_LayerSequence nnStructure[] = {{.layerType = model.LAYER_INPUT, .neurons =  0, .activationType = model.ACTIVATION_NONE},  // input layer
                                        {.layerType = model.LAYER_DENSE, .neurons = 10, .activationType = model.ACTIVATION_RELU},  // hidden layer
                                        {.layerType = model.LAYER_DENSE, .neurons = 10, .activationType = model.ACTIVATION_RELU},  // hidden layer
                                        {.layerType = model.LAYER_DENSE, .neurons =  1, .activationType = model.ACTIVATION_RELU}}; // output layer          
    modelPara.inputLayerPara = FLAG_MODEL_2D_INPUT_LAYER_DIM(data->featureDim);
    modelPara.layerSize = FLAG_MODEL_GET_LAYER_SIZE(nnStructure);                    
    modelPara.layerSeq = nnStructure;
    modelPara.lossFuncType  = model.LOSS_FUNC_MSE;
    modelPara.optimizerPara = {.optimizerType = model.OPTIMIZER_ADAM, .learningRate = 0.001, .epochs = 100, .batch_size = 10};
    model.begin(&modelPara);
  } 
};

// 訓練模型
Flag_Task trainModel = {
  .active = true, 
  .period = taskManager.ONE_SHOT, 
  .callback = [](){
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
};

// 評估模型
Flag_Task evalModel = {
  .active = true, 
  .period = taskManager.ONE_SHOT, 
  .callback = [](){
    // 使用訓練好的模型來預測
    float eval_feature_data;  
    uint16_t eval_feature_shape[] = {1, data->featureDim}; 
    aitensor_t eval_feature_tensor = AITENSOR_2D_F32(eval_feature_shape, &eval_feature_data);
    aitensor_t *eval_output_tensor;
    float predictVal;

    Serial.println("量測值:\t\t預測值:");
    for(int i = 0; i >= -1000; i--){ //i是模擬HX711量測到的值
      // 測試資料預處理
      eval_feature_data = (i - mean) / sd;

      // 模型預測
      eval_output_tensor = model.predict(&eval_feature_tensor); 
      
      // 輸出預測結果
      model.getResult(eval_output_tensor, labelMaxAbs, &predictVal);  //因為標籤資料有經過正規化, 所以要將label的比例還原回來
      Serial.print(i);
      Serial.print(F("\t\t"));
      model.printResult(&predictVal);
    }
  }
};

void setup() {
  // UART設置
  Serial.begin(115200);

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

  // 順序增加Task
  taskManager.add(&createModel); //增加第1個task
  taskManager.add(&trainModel);  //增加第2個task
  taskManager.add(&evalModel);   //增加第3個task
  
  Serial.println(F("----- 訓練<HX711 - 重量>特性圖範例 -----"));
  Serial.println();
}

void loop() {
  taskManager.execute();
}



// 確認讀取到的資料
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