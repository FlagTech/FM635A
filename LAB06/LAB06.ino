/*
  電子秤 -- 即時預測
*/
#include <Flag_Model.h>
#include <Flag_HX711.h>

// ------------全域變數------------
// 神經網路模型
Flag_Model model; 

// 感測器的物件
Flag_HX711 hx711(32, 33);
// -------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // HX711 初始化
  hx711.begin();

  // 扣重調整
  hx711.tare();

  // ----------------- 建構模型 --------------------
  // 讀取已訓練好的模型檔
  model.begin("/weight_model.json");
}

void loop() {
  // ----------------- 即時預測 --------------------
  // 測試資料預處理
  float test_feature_data = 
    (hx711.getWeight() - model.mean) / model.sd;

  // 模型預測
  uint16_t test_feature_shape[] = {
    1, // 每次測試一筆資料
    model.inputLayerDim
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
  Serial.print("重量: ");
  model.getResult(
    test_output_tensor,
    model.labelMaxAbs,
    &predictVal
  );
  Serial.print(predictVal, 1); 
  Serial.println('g');
}
