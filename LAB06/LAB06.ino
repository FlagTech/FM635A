/*
  電子秤 -- 即時預測
*/
#include <Flag_DataReader.h>
#include <Flag_Model.h>
#include <Flag_HX711.h>

// ------------全域變數------------
// 讀取資料的物件
Flag_DataReader reader;

// 指向存放資料的指位器
Flag_DataBuffer *data;

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

  // 歸零調整
  hx711.tare();

  // ------------------------- 資料預處理 -------------------------
  // 迴歸類型的訓練資料讀取
  data = reader.read(
    "/dataset/weight.txt", 
    reader.MODE_REGRESSION
  );

  // 取得訓練特徵資料的平均值
  mean = data->featureMean;

  // 取得訓練特徵資料的標準差
  sd = data->featureSd;

  // 取得最大訓練標籤資料的絕對值
  labelMaxAbs = data->labelMaxAbs; 

  // -------------------------- 建構模型 --------------------------
  // 讀取已訓練好的模型檔
  model.begin("/weight_model.json");
}

void loop() {
  // -------------------------- 即時預測 --------------------------
  // 測試資料預處理
  float test_feature_data = (hx711.getWeight() - mean) / sd;

  // 模型預測
  uint16_t test_feature_shape[] = {
    1, // 每次測試一筆資料
    data->featureDim
  }; 
  aitensor_t test_feature_tensor = AITENSOR_2D_F32(
    test_feature_shape, 
    &test_feature_data
  );

  aitensor_t *test_output_tensor;
  float predictVal;

  test_output_tensor = model.predict(&test_feature_tensor); 

  // 輸出預測結果
  Serial.print("重量: ");
  model.getResult(test_output_tensor, labelMaxAbs, &predictVal);
  Serial.print(predictVal, 1); 
  Serial.println('g');
}
