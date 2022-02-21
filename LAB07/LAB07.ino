/*
  電子料理秤 -- 即時預測範例
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>

// ------------全域變數------------
Flag_DataReader reader;
Flag_DataBuffer *data;
Flag_Model model; 

// 資料預處理會用到的參數
float mean;
float sd;
float labelMax;
// -------------------------------

void setup() {
  // UART config
  Serial.begin(115200);

  // 讀取已訓練的模型檔
  model.begin("/weight_model.json");

  // 回歸類型的資料讀取
  data = reader.read("/dataset/weight.txt", reader.MODE_REGRESSION);

  // 取得特徵資料的平均值
  mean = data->featureMean;

  // 取得特徵資料的標準差
  sd = data->featureSd;

  // 取得標籤資料的最大絕對值
  labelMax = data->labelMax; 
        
  Serial.println(F("----- 即時預測重量 -----"));
  Serial.println();
}

void loop() {
  // -------------------------- 即時預測 --------------------------
  // 使用訓練好的模型來預測
  float test_feature_data[1];  
  uint16_t test_feature_shape[] = {1, data->featureDim}; 
  aitensor_t test_feature_tensor = AITENSOR_2D_F32(test_feature_shape, test_feature_data);
  aitensor_t *test_output_tensor;

  Serial.println("量測值:\t\t預測值:");
  for(int i = 0; i >= -1000; i--){ //i是HX711量測到的值
    //正規化: 標準差法
    test_feature_data[0] = (i - mean) / sd;
    test_output_tensor = model.predict(&test_feature_tensor);
    Serial.print(i);
    Serial.print(F("\t\t"));
    Serial.println(((float* ) test_output_tensor->data)[0] * labelMax); //因為標籤資料有經過正規化, 所以要*1000將比例還原回來
  }

  while(1);
}
