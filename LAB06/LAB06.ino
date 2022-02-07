 /*
  電子料理秤 -- 訓練與預測範例
  --------------------
  該草圖展現如何在AIfES中使用訓練數據來訓練神經網絡。
  網絡結構為1-10(relu))-10(relu)-1(relu)，使用relu作為激活函數。
  使用Glorot初始化。
  使用ADAM優化器的訓練。
  優化器執行1900個 epoch 的批量訓練。
  計算在 float 32 中完成。
*/
#include <FM635A_Utilities.h>
#include <FM635A_ModelLite.h>

//全域變數
struct{
  FM635A_ModelLite::DataReader reader;
  FM635A_ModelLite::Model model; 
  FM635A_Utilities::Statistics math;
  float *data;
  float *label;
  uint32_t dataLen;
  uint32_t featureInputNum;
  uint32_t labelInputNum;
  uint32_t labelDataLen;
  uint32_t featureDataLen;
  float mean;
  float std;
}globalVar;

void setup() {
  //UART config
  Serial.begin(115200);

  //ADC config
  analogSetAttenuation(ADC_11db); // 衰減11db, 目的是可以量測到3.3v
  analogSetWidth(10);             // 10位元解析度

  //回歸類型的資料讀取
  globalVar.reader.debugInfoTypeConfig(FM635A_ModelLite::INFO_SIMPLE);
  globalVar.reader.config(FM635A_ModelLite::MCU_SUPPORT_ESP32, "/dataset/weight.txt", FM635A_ModelLite::MODE_REGRESSION);
  globalVar.reader.read();
  
  globalVar.data = globalVar.reader.regressionData.feature;                      //get data
  globalVar.label = globalVar.reader.regressionData.label;                       //get label
  globalVar.dataLen = globalVar.reader.regressionData.dataLen;                   //資料筆數
  globalVar.featureInputNum = globalVar.reader.regressionData.featureInputNum;   //特徵輸入維度
  globalVar.labelInputNum = globalVar.reader.regressionData.labelInputNum;       //label維度
  globalVar.featureDataLen = globalVar.reader.regressionData.featureDataArryLen; //data的陣列長度(以1維陣列來看)
  globalVar.labelDataLen = globalVar.reader.regressionData.labelDataArryLen;     //label的陣列長度(以1維陣列來看)

  //回歸類型的資料確認
  for(int j = 0; j < globalVar.reader.regressionData.dataLen; j++){
    Serial.print(F("Feature Data :"));
    for(int i = 0; i < globalVar.reader.regressionData.featureInputNum; i++){
      Serial.print(globalVar.data[i + j * globalVar.reader.regressionData.featureInputNum]);
      if(i != globalVar.reader.regressionData.featureInputNum - 1){
        Serial.print(F(","));
      }
    }
    Serial.print(F("\t\t"));

    Serial.print("Label Data :");
    for(int i = 0; i < globalVar.reader.regressionData.labelInputNum; i++){
      Serial.print(globalVar.label[i +  j * globalVar.reader.regressionData.labelInputNum]);
      if(i != globalVar.reader.regressionData.labelInputNum - 1){
        Serial.print(F(","));
      }
    }
    Serial.println(F(""));
  }

  //重要!!!
  //AIfES 需要隨機權重進行訓練
  //這裡的隨機種子是由類比引腳的雜訊產生的
  uint32_t aRead;
  aRead = analogRead(A5);
  srand(104);

  // 資料預處理, data使用正規化之標準差化
  // 計算data平均值
  globalVar.math.mean_data_config.dataLen = globalVar.featureDataLen; 
  globalVar.math.mean_data_config.dataType = FM635A_Utilities::DATA_TYPE_F32;
  globalVar.math.mean_data_config.paraBuf = globalVar.data;
  globalVar.mean = globalVar.math.mean();
  Serial.print("mean: ");
  Serial.println(globalVar.mean);

  // 計算data標準差
  globalVar.math.std_data_config.dataLen = globalVar.featureDataLen;
  globalVar.math.std_data_config.stdType = FM635A_Utilities::STATISTICS_POPULATION;
  globalVar.math.std_data_config.dataType = FM635A_Utilities::DATA_TYPE_F32;
  globalVar.math.std_data_config.paraBuf = globalVar.data;
  globalVar.std = globalVar.math.std();
  Serial.print("std: ");
  Serial.println(globalVar.std);
        
  //特徵資料正規化: 標準差法
  for(int j = 0; j < globalVar.featureDataLen; j++){
    globalVar.data[j] = (globalVar.data[j] - globalVar.mean) / globalVar.std;
  }

  //標籤資料正規化: Min/Max法
  for(int j = 0; j < globalVar.labelDataLen; j++){
    globalVar.label[j] /= 1000; //最大值為1000g
  }
  
  Serial.println(F("HX711 - Weight training demo"));
  Serial.println(F("Type >training< to start"));
}

void loop() {

  while(Serial.available() > 0 ){
    String str = Serial.readString();
    if(str.indexOf("training") > -1)
    {
        // 取得訓練用的特徵張量
        float *input_training_data = globalVar.data;
        uint16_t input_training_shape[] = {globalVar.dataLen, globalVar.featureInputNum};
        aitensor_t input_training_tensor = AITENSOR_2D_F32(input_training_shape, input_training_data);
    
        // 取得訓練用的標籤張量
        float *target_data = globalVar.label;
        uint16_t target_shape[] = {globalVar.dataLen, globalVar.labelInputNum}; 
        aitensor_t target_training_tensor = AITENSOR_2D_F32(target_shape, target_data); 

        //定義神經層
        FM635A_ModelLite::modelTrainParameter modelTrainPara;
        modelTrainPara.input_tensor  = &input_training_tensor;
        modelTrainPara.target_tensor = &target_training_tensor;
        modelTrainPara.layerSize = 4;
        FM635A_ModelLite::layerSequence nnStructure[] = {{FM635A_ModelLite::LAYER_INPUT,  0, FM635A_ModelLite::ACTIVATION_NONE},  // input layer
                                                         {FM635A_ModelLite::LAYER_DENSE, 10, FM635A_ModelLite::ACTIVATION_RELU},  // hidden layer
                                                         {FM635A_ModelLite::LAYER_DENSE, 10, FM635A_ModelLite::ACTIVATION_RELU},  // hidden layer
                                                         {FM635A_ModelLite::LAYER_DENSE,  1, FM635A_ModelLite::ACTIVATION_RELU},};// output layer
                                                        
        modelTrainPara.layerSeq = nnStructure;
        modelTrainPara.lossFuncType  = FM635A_ModelLite::LOSS_FUNC_MSE;
        modelTrainPara.optimizerPara = {FM635A_ModelLite::OPTIMIZER_ADAM, 0.001, 100, 10}; //optimizer type, learning rate, epochs, batch-size
        
        //訓練模型 
        globalVar.model.train(&modelTrainPara);

        // ----------------------------------------- 評估模型 --------------------------
        // 使用訓練好的模型來預測
        uint16_t input_test_shape[] = {1, 1}; 
        float input_test_data[1];  
        aitensor_t input_test_tensor = AITENSOR_2D_F32(input_test_shape, input_test_data);
        aitensor_t *output_test_tensor;
        Serial.print("HX711量測值  模型預測值:");
        for(int i = 0; i >= -1000; i--){ //i是HX711量測到的值
          //正規化: 標準差法
          input_test_data[0] = (i - globalVar.mean) / globalVar.std;
          output_test_tensor = globalVar.model.predict(&input_test_tensor);
          Serial.print(i);
          Serial.print(" ");
          Serial.println(((float* ) output_test_tensor->data)[0] * 1000.0); //因為標籤資料有經過正規化, 所以要*1000將比例還原回來
        }
        while(1);
     }else{
      Serial.println(F("unknown"));
    }
  }
}
