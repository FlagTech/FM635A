/*
  電子料理秤 -- 即時預測
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <HX711.h>

// ------------全域變數------------
// 讀取資料的物件
Flag_DataReader reader;
Flag_DataBuffer *data;

// 神經網路模型
Flag_Model model; 

// 感測器的物件
HX711 hx(16, 17, 128, -0.0011545); 

// 資料預處理會用到的參數
float mean;
float sd;
float labelMaxAbs;
// -------------------------------

void setup() {
  // UART config
  Serial.begin(115200);

  // 回歸類型的資料讀取
  data = reader.read("/dataset/weight.txt", reader.MODE_REGRESSION);

  // 取得特徵資料的平均值
  mean = data->featureMean;

  // 取得特徵資料的標準差
  sd = data->featureSd;

  // 取得標籤資料的最大絕對值
  labelMaxAbs = data->labelMaxAbs; 
        
  Serial.println(F("----- 即時預測重量 -----"));
  Serial.println();

  // -------------------------- 建構模型 --------------------------
  // 讀取已訓練的模型檔
  model.begin("/weight_model.json");

}

void loop() {
  // -------------------------- 即時預測 --------------------------
  // 使用訓練好的模型來預測
  float test_feature_data;  
  uint16_t test_feature_shape[] = {1, data->featureDim}; 
  aitensor_t test_feature_tensor = AITENSOR_2D_F32(test_feature_shape, &test_feature_data);
  aitensor_t *test_output_tensor;
  float predictVal;

  Serial.println("量測值:\t\t預測值:");
  for(int i = 0; i >= -1000; i--){ //i是HX711量測到的值
    // 測試資料預處理
    test_feature_data = (i - mean) / sd;

    // 模型預測
    test_output_tensor = model.predict(&test_feature_tensor); 

    // 輸出預測結果
    model.getResult(test_output_tensor, labelMaxAbs, &predictVal);  //因為標籤資料有經過正規化, 所以要將label的比例還原回來
    Serial.print(i);
    Serial.print(F("\t\t"));
    model.printResult(&predictVal);
  }

  while(1);
}
