 /*
  呼吸燈wave訓練範例
  --------------------
  該草圖展現如何在AIfES中使用訓練數據來訓練神經網絡。
  網絡結構為1-10(tanh)-10(tanh)-1(tanh)，使用tanh作為激活函數。
  使用Glorot初始化。
  使用ADAM優化器的訓練。
  優化器執行1900個 epoch 的批量訓練。
  計算在 float 32 中完成。
*/
#include "./inc/feature.h"
#include "./inc/label.h"
#include <FM635A_Utilities.h>
#include <aifes.h>                  

// 角度轉弳度
#define D2R(deg) (deg * PI / 180.0)

// 特徵資料樣本數
#define SAMPLE_TOTAL (sizeof(feature_degree)/sizeof(feature_degree[0]))

//----------特徵資料----------
// 準備好訓練用的特徵資料
float *input_data = feature_degree;

// 定義訓練用的特徵資料的shape
uint16_t input_shape[] = {SAMPLE_TOTAL, feature_dim};    
//---------------------------

//----------標籤資料----------
// 對應特徵資料的標籤值
float *target_data = label_wave;

// 定義訓練用的標籤資料的shape
uint16_t target_shape[] = {SAMPLE_TOTAL, label_dim};
//---------------------------


// 會用到的統計物件
FM635A_Utilities::Statistics math;
float gMean;
float gStd;

void setup() {
  //UART config
  Serial.begin(115200);

  //PWM config
  ledcSetup(0, 5000, 10);          // PWM通道, 頻率, 解析度
  ledcAttachPin(LED_BUILTIN, 0);
  
  //ADC config
  analogSetAttenuation(ADC_11db); // 衰減11db, 目的是可以量測到3.3v
  analogSetWidth(10);             // 10位元解析度
  
  //重要!!!
  //AIfES 需要隨機權重進行訓練
  //這裡的隨機種子是由類比引腳的雜訊產生的
  uint16_t randSeed = analogRead(A5); // 取得亂數種子
  srand(641);
  
  // 資料預處理, data使用正規化之標準差化
  // 計算data平均值
  math.mean_data_config.dataLen = SAMPLE_TOTAL; 
  math.mean_data_config.dataType = FM635A_Utilities::DATA_TYPE_F32;
  math.mean_data_config.paraBuf = input_data;
  gMean = math.mean();
  Serial.print("mean: ");
  Serial.println(gMean);

  // 計算data標準差
  math.std_data_config.dataLen = SAMPLE_TOTAL;
  math.std_data_config.stdType = FM635A_Utilities::STATISTICS_POPULATION;
  math.std_data_config.dataType = FM635A_Utilities::DATA_TYPE_F32;
  math.std_data_config.paraBuf = input_data;
  gStd = math.std();
  Serial.print("std: ");
  Serial.println(gStd);
  
  //特徵資料正規化: 標準差法
  for(int i = 0; i < SAMPLE_TOTAL; i++){
    input_data[i] = (input_data[i] - gMean) / gStd;
  }

  //標籤資料正規化: Min/Max法
  for(int i = 0; i < SAMPLE_TOTAL; i++){
    target_data[i] = target_data[i] / 1.0;
  }

  // 傳送training來進行訓練
  Serial.println(F("Sine wave training demo"));
  Serial.println(F("Type >training< to start"));
}

void loop() {

  while(Serial.available() > 0 ){
    String str = Serial.readString();
    if(str.indexOf("training") > -1)
    {
      Serial.println(F("AIfES:"));
      Serial.println(F(""));
      Serial.print(F("rand test: "));
      Serial.println(rand());
      Serial.println();

      // 取得訓練用的特徵張量
      aitensor_t input_tensor = AITENSOR_2D_F32(input_shape, input_data);

      // 取得訓練用的標籤張量
      aitensor_t target_tensor = AITENSOR_2D_F32(target_shape, target_data); 
      
      // 準備一個輸出張量用來存結果
      uint16_t output_shape[] = {SAMPLE_TOTAL, 1};
      float output_data[SAMPLE_TOTAL];
      aitensor_t output_tensor = AITENSOR_2D_F32(output_shape, output_data);
      
      // ---------------------------------- 定義神經層(於相當於模型的零組件) ---------------------------------------
      uint16_t input_layer_shape[] = {1, input_tensor.shape[1]}; //第一個元素參考官方範例, 固定為1; 第二個元素為input tensor shape[1], 即input data的dim(維度)
      ailayer_input_f32_t   input_layer     = AILAYER_INPUT_F32_A(input_tensor.dim, input_layer_shape); 
      ailayer_dense_f32_t   dense_layer_1   = AILAYER_DENSE_F32_A(10); // 第一層隱藏層, 使用10個神經元
      ailayer_tanh_f32_t    tanh_layer_1    = AILAYER_TANH_F32_A();    // 第一層隱藏層的激勵函數, 因為是sine wave的回歸問題, 使用tanh處理是很好的選擇
      ailayer_dense_f32_t   dense_layer_2   = AILAYER_DENSE_F32_A(10); // 第一層隱藏層, 使用10個神經元
      ailayer_tanh_f32_t    tanh_layer_2    = AILAYER_TANH_F32_A();    // 第一層隱藏層的激勵函數, 因為是sine wave的回歸問題, 使用tanh處理是很好的選擇
      ailayer_dense_f32_t   dense_layer_3   = AILAYER_DENSE_F32_A(1);  // 輸出層, 使用1個神經元
      ailayer_tanh_f32_t    tanh_layer_3    = AILAYER_TANH_F32_A();    // 輸出層的激勵函數, 因為是sine wave的回歸問題, 使用tanh處理是很好的選擇

      ailoss_mse_t mse_loss;  //損失函數(Loss Function): 均方誤差
      

      // --------------------------- 定義模型結構 ----------------------------
      aimodel_t model;  // 宣告AIfES模型
      ailayer_t *x;     // 為了連結各層神經層過渡用的
      
      // 使用上述定義的神經層組件來加入到模型, 其中會使用到過渡用的神經層x
      model.input_layer = ailayer_input_f32_default(&input_layer);
      x = ailayer_dense_f32_default(&dense_layer_1, model.input_layer);
      x = ailayer_tanh_f32_default(&tanh_layer_1, x);
      x = ailayer_dense_f32_default(&dense_layer_2, x);
      x = ailayer_tanh_f32_default(&tanh_layer_2, x);
      x = ailayer_dense_f32_default(&dense_layer_3, x);
      x = ailayer_tanh_f32_default(&tanh_layer_3, x);
      model.output_layer = x;
      
      // 定義損失函數, 此例為回歸問題, 使用均方誤差
      model.loss = ailoss_mse_f32_default(&mse_loss, model.output_layer);
      
      aialgo_compile_model(&model); // 編譯AIfES模型
      

      // ------------------------------------- 顯示模型結構 ------------------------------------
      Serial.println(F("-------------- Model structure ---------------"));
      Serial.println(F("Layer:"));
      aialgo_print_model_structure(&model);
      Serial.print(F("\nLoss: "));
      aialgo_print_loss_specs(model.loss);
      Serial.println(F("\n----------------------------------------------\n"));
      

      // ------------------------------- 模型權重參數的記憶體配置 ------------------------------
      uint32_t parameter_memory_size = aialgo_sizeof_parameter_memory(&model);
      Serial.print(F("Required memory for parameter (Weights, Biases): "));
      Serial.print(parameter_memory_size);
      Serial.print(F(" bytes"));
      Serial.println();
      byte *parameter_memory = (byte *) malloc(parameter_memory_size);

      // 分配記憶體給模型的訓練參數
      aialgo_distribute_parameter_memory(&model, parameter_memory, parameter_memory_size);
      

      // ------------------------------- 權重參數的初始化 ------------------------------
      aimath_f32_default_init_glorot_uniform(&dense_layer_1.weights);
      aimath_f32_default_init_zeros(&dense_layer_1.bias);
      
      aimath_f32_default_init_glorot_uniform(&dense_layer_2.weights);
      aimath_f32_default_init_zeros(&dense_layer_2.bias);

      aimath_f32_default_init_glorot_uniform(&dense_layer_3.weights);
      aimath_f32_default_init_zeros(&dense_layer_3.bias);

      // -------------------------------- 定義優化器---------------------
      aiopti_adam_f32_t adam_opti = AIOPTI_ADAM_F32(0.001f, 0.9f, 0.999f, 1e-7);
      aiopti_t *optimizer = aiopti_adam_f32_default(&adam_opti);
      
      
      // -------------------------------- 配置訓練用的記憶體 ---------
      uint32_t memory_size = aialgo_sizeof_training_memory(&model, optimizer);
      Serial.print(F("Required memory for the training (Intermediate results, gradients, optimization memory): "));
      Serial.print(memory_size);
      Serial.print(F(" bytes"));
      Serial.println();
      byte *memory_ptr = (byte *) malloc(memory_size);

      if(memory_ptr == 0){
        Serial.println(F("ERROR: Not enough memory (RAM) available for training! Try to use another optimizer (e.g. SGD) or make your net smaller."));
        while(1);
      }
      
      // 為了訓練模型而分配的記憶體
      aialgo_schedule_training_memory(&model, optimizer, memory_ptr, memory_size);
      
      // 重要：在訓練之前需先初始化AIfES模型
      aialgo_init_model_for_training(&model, optimizer);
      

      // --------------------------------- 訓練前先印訊息, 看看未訓練的模型其預測結果為何 ----------------------------------
      // 訓練前的正向傳播
      aialgo_inference_model(&model, &input_tensor, &output_tensor);
      
      Serial.println();
      Serial.println(F("Before training:"));
      Serial.println(F("Results:"));
      Serial.println(F("input 1:\treal output:\tcalculated output:"));
      
      for (int i = 0; i < SAMPLE_TOTAL; i++) {
        Serial.print(input_data[i]);
        Serial.print(F("\t\t"));

        Serial.print(target_data[i]);
        Serial.print(F("\t\t"));

        Serial.println(output_data[i]);
      }


      // ------------------------------------- 訓練參數配置 ------------------------------------
      uint32_t batch_size = 4;
      uint16_t epochs = 1900;
      uint16_t print_interval = 10;
      
      Serial.println(F("\n------------ Training configuration ----------"));
      Serial.print(F("Epochs: "));
      Serial.print(epochs);
      Serial.print(F(" (Print loss every "));
      Serial.print(print_interval);
      Serial.println(F(" epochs)"));
      Serial.print(F("Batch size: "));
      Serial.println(batch_size);
      Serial.print(F("Optimizer: "));
      aialgo_print_optimizer_specs(optimizer);
      Serial.println(F("\n----------------------------------------------\n"));
      

      // ------------------------------------- 訓練模型 ------------------------------------
      Serial.println(F("Start training"));
      float loss;
      for(int i = 0; i < epochs; i++)
      {
        // 一個epoch就遍歷整個資料集一次
        aialgo_train_model(&model, &input_tensor, &target_tensor, optimizer, batch_size);
      
        // 每print_interval個epoch, 印一次損失函數的值
        if(i % print_interval == 0)
        {
          aialgo_calc_loss_model_f32(&model, &input_tensor, &target_tensor, &loss);
          Serial.print(F("Epoch: "));
          Serial.print(i);
          Serial.print(F(" Loss: "));
          Serial.println(loss);
        }
      }
      Serial.println(F("Finished training"));
      
      // -------------------------- 評估模型 --------------------------
      //訓練後的正向傳播
      aialgo_inference_model(&model, &input_tensor, &output_tensor);
      
      Serial.println(F(""));
      Serial.println(F("After training:"));
      Serial.println(F("Results:"));
      Serial.println(F("input 1:\treal output:\tcalculated output:"));
      
      for (int i = 0; i < SAMPLE_TOTAL; i++) {
        Serial.print (input_data[i]);
        Serial.print(F("\t\t"));

        Serial.print(target_data[i]);
        Serial.print(F("\t\t"));

        Serial.println(output_data[i]);
      }

      // 使用訓練好的模型來建立呼吸燈
      uint16_t input_test_shape[] = {1, feature_dim};  
      float input_test_data[label_dim];
      aitensor_t input_test_tensor  = AITENSOR_2D_F32(input_test_shape, input_test_data);
      aitensor_t *output_test_tensor;
      float predictVal = 0;

      while(1){
        for(int i = 0; i < 720; i++){
          input_test_data[0] = (i - gMean) / gStd;
          output_test_tensor = aialgo_forward_model(&model, &input_test_tensor); // 正向傳播

          predictVal= ((float* ) output_test_tensor->data)[0]; 
          ledcWrite(0, 512 * predictVal + 511); 
          delay(5);
        }     
      }

      free(parameter_memory);
      free(memory_ptr);

    } else{
      Serial.println(F("unknown"));
    }
  }

}
