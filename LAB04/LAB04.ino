 /*
  第一個機器學習模型 -- 呼吸燈wave訓練
  --------------------
  該草圖展現如何在AIfES中使用訓練數據來訓練神經網絡。
    -網絡結構為1-10(tanh)-10(tanh)-1(tanh)，使用tanh作為激活函數。
    -使用Glorot初始化。
    -使用ADAM優化器的訓練。
    -優化器執行1900個 epoch 的批量訓練。
    -計算在 float 32 中完成。
*/

#include "./inc/feature.h"
#include "./inc/label.h"
#include <Flag_Statistics.h>
#include <aifes.h>                  

// ------------全域變數------------
// 訓練用的特徵資料
float *train_feature_data;  

// 對應特徵資料的標籤值
float *train_label_data;

// 資料預處理會用到的參數
float mean;
float sd;
float labelMax;
// -------------------------------

void setup() {
  // UART設置
  Serial.begin(115200);

  // PWM設置
  ledcSetup(0, 5000, 10);          // PWM通道, 頻率, 解析度
  ledcAttachPin(LED_BUILTIN, 0);
  
  // ADC設置
  analogSetAttenuation(ADC_11db); // 衰減11db, 目的是可以量測到3.3v
  analogSetWidth(10);             // 10位元解析度
  
  // 重要!!! AIfES需要隨機權重進行訓練, 這裡的隨機種子是由類比引腳的雜訊產生的
  uint16_t randSeed = analogRead(A5); // 取得亂數種子
  srand(randSeed);                    // srand(641); 很穩
  
  // 設定訓練用的特徵資料
  train_feature_data = (float*) malloc(sizeof(float) * SAMPLE_TOTAL);
  for(int i = 0; i < SAMPLE_TOTAL; i++){
    train_feature_data[i] = degree[i];
  }

  // 設定對應特徵資料的標籤值
  train_label_data = (float*) malloc(sizeof(float) * SAMPLE_TOTAL);
  for(int i = 0; i < SAMPLE_TOTAL; i++){
    train_label_data[i] = wave_val[i];
  }

  // 資料預處理會用到的統計數學物件
  Flag_Statistics stats;

  // 計算特徵資料的平均值
  mean = stats.mean(train_feature_data, SAMPLE_TOTAL);

  // 計算特徵資料的標準差
  sd = stats.sd(train_feature_data, SAMPLE_TOTAL);

  // 取得標籤資料的最大絕對值
  labelMax = wave_val_max; 

  // 特徵資料正規化: 標準差法
  for(int i = 0; i < SAMPLE_TOTAL; i++){
    train_feature_data[i] = (train_feature_data[i] - mean) / sd;
  }

  // 標籤資料正規化: Min/Max法
  for(int i = 0; i < SAMPLE_TOTAL; i++){
    train_label_data[i] = train_label_data[i] / labelMax;
  }

  // 傳送training字串來進行訓練
  Serial.println("----- 訓練wave函數範例 -----");
  Serial.println("請輸入training來開始訓練");
  Serial.println();
}

void loop() {
  while(Serial.available() > 0 ){
    String str = Serial.readString();
    if(str.indexOf("training") > -1)
    {
      // 依據先前所設定的亂數種子來取亂數
      rand();
      
      // 創建訓練用的特徵張量
      uint16_t train_feature_shape[] = {SAMPLE_TOTAL, feature_dim};   
      aitensor_t train_feature_tensor = AITENSOR_2D_F32(train_feature_shape, train_feature_data);

      // 創建訓練用的標籤張量
      uint16_t train_label_shape[] = {SAMPLE_TOTAL, label_dim};
      aitensor_t train_label_tensor = AITENSOR_2D_F32(train_label_shape, train_label_data); 
      
      // ---------------------------------- 定義神經層(於相當於模型的零組件) ---------------------------------------
      uint16_t input_layer_shape[] = {1, train_feature_tensor.shape[1]}; // 第一個元素參考官方範例, 固定為1; 第二個元素為train_feature_tensor.shape[1], 即訓練data的dim(維度)
      ailayer_input_f32_t   input_layer   = AILAYER_INPUT_F32_A(train_feature_tensor.dim, input_layer_shape); 
      ailayer_dense_f32_t   dense_layer_1 = AILAYER_DENSE_F32_A(10); // 第一層隱藏層, 使用10個神經元
      ailayer_tanh_f32_t    tanh_layer_1  = AILAYER_TANH_F32_A();    // 第一層隱藏層的激勵函數, 因為是sine wave的回歸問題, 使用tanh處理是很好的選擇
      ailayer_dense_f32_t   dense_layer_2 = AILAYER_DENSE_F32_A(10); // 第一層隱藏層, 使用10個神經元
      ailayer_tanh_f32_t    tanh_layer_2  = AILAYER_TANH_F32_A();    // 第一層隱藏層的激勵函數, 因為是sine wave的回歸問題, 使用tanh處理是很好的選擇
      ailayer_dense_f32_t   dense_layer_3 = AILAYER_DENSE_F32_A(1);  // 輸出層, 使用1個神經元
      ailayer_tanh_f32_t    tanh_layer_3  = AILAYER_TANH_F32_A();    // 輸出層的激勵函數, 因為是sine wave的回歸問題, 使用tanh處理是很好的選擇

      ailoss_mse_t mse_loss;  // 因為是回歸問題, 損失函數(Loss Function)選擇MSE(均方誤差)
      
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
      
      // 加入損失函數, 此例為回歸問題, 使用MSE(均方誤差)
      model.loss = ailoss_mse_f32_default(&mse_loss, model.output_layer);
      
      // 編譯AIfES模型
      aialgo_compile_model(&model); 
      
      // ------------------------------------- 顯示模型結構 ------------------------------------
      Serial.println(F("-------------- 模型結構 ---------------"));
      Serial.println(F("Layer:"));
      aialgo_print_model_structure(&model);
      Serial.print(F("\nLoss: "));
      aialgo_print_loss_specs(model.loss);
      Serial.println(F("\n--------------------------------------"));
      Serial.println();

      // ------------------------------- 模型權重參數的記憶體配置 ------------------------------
      // 權重參數（Weights, Biases）所需的記憶體；單位：byte
      uint32_t parameter_memory_size = aialgo_sizeof_parameter_memory(&model); 
      byte *parameter_memory = (byte *) malloc(parameter_memory_size);

      if(parameter_memory == 0){
        Serial.println(F("錯誤：沒有足夠的記憶體(RAM)可用於權重參數（Weights, Biases）"));
        while(1);
      }

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
      // 訓練時所需的記憶體（中間結果、梯度、優化內存）；單位：字節
      uint32_t memory_size = aialgo_sizeof_training_memory(&model, optimizer); 
      byte *memory_ptr = (byte *) malloc(memory_size);

      if(memory_ptr == 0){
        Serial.println(F("錯誤：沒有足夠的記憶體(RAM)可用於訓練, 嘗試使用其他優化器(例如SGD）或縮小您的神經網路"));
        while(1);
      }
      
      // 為了訓練模型而分配的記憶體
      aialgo_schedule_training_memory(&model, optimizer, memory_ptr, memory_size);
      
      // 重要：在訓練之前需先初始化AIfES模型
      aialgo_init_model_for_training(&model, optimizer);

      // ------------------------------------- 訓練參數配置 ------------------------------------
      uint32_t batch_size = 4;
      uint16_t epochs = 1900;
      uint16_t print_interval = 10;
      
      Serial.println(F("------------ 訓練參數配置 ----------"));
      Serial.print(F("Epochs: "));
      Serial.print(epochs);
      Serial.print(F("每"));
      Serial.print(print_interval);
      Serial.println(F("個Epochs印一次Loss值"));
      Serial.print(F("Batch size: "));
      Serial.println(batch_size);
      Serial.print(F("Optimizer: "));
      aialgo_print_optimizer_specs(optimizer);
      Serial.println(F("\n----------------------------------"));
      Serial.println();

      // ------------------------------------- 訓練模型 ------------------------------------
      Serial.println(F("--------- 訓練開始 ---------"));
      float loss;
      for(int i = 0; i < epochs; i++)
      {
        // 一個epoch就遍歷整個資料集一次
        aialgo_train_model(&model, &train_feature_tensor, &train_label_tensor, optimizer, batch_size);
      
        // 每print_interval個epoch, 印一次損失函數的值
        if(i % print_interval == 0)
        {
          aialgo_calc_loss_model_f32(&model, &train_feature_tensor, &train_label_tensor, &loss);
          Serial.print(F("Epoch: "));
          Serial.print(i);
          Serial.print(F("\tLoss: "));
          Serial.println(loss);
        }
      }
      Serial.println(F("--------- 訓練結束 ---------"));
      Serial.println();
      
      // -------------------------- 評估模型 --------------------------
      // 訓練後的正向傳播
      Serial.println(F("訓練完成的結果:"));
      Serial.println(F("特徵輸入:\t\t實際值:\t\t預測值:"));

      // 比較模型預測值與實際值
      float eval_feature_data[feature_dim];
      uint16_t eval_feature_shape[] = {1, feature_dim};  
      aitensor_t eval_feature_tensor  = AITENSOR_2D_F32(eval_feature_shape, eval_feature_data);
      aitensor_t *eval_output_tensor;

      for (int i = 0; i < SAMPLE_TOTAL; i++) {
        eval_feature_data[0] = train_feature_data[i];

        eval_output_tensor = aialgo_forward_model(&model, &eval_feature_tensor);
        Serial.print (train_feature_data[i]);
        Serial.print(F("\t\t"));

        Serial.print(train_label_data[i] * labelMax); //需將預測值乘回縮放比例, 才是原始標籤值
        Serial.print(F("\t\t"));

        Serial.println(((float* ) eval_output_tensor->data)[0] * labelMax); //需將預測值乘回縮放比例, 才是原始標籤值
      }

      // -------------------------- 使用訓練好的模型來建立呼吸燈 -------------------------- 
      float test_feature_data[feature_dim];
      uint16_t test_feature_shape[] = {1, feature_dim};  
      aitensor_t test_feature_tensor  = AITENSOR_2D_F32(test_feature_shape, test_feature_data);
      aitensor_t *test_output_tensor;
      float predictVal = 0;

      while(1){
        for(int i = 0; i < degree[SAMPLE_TOTAL-1]; i++){
          test_feature_data[0] = (i - mean) / sd;
          test_output_tensor = aialgo_forward_model(&model, &test_feature_tensor); // 正向傳播

          predictVal = ((float* ) test_output_tensor->data)[0] * labelMax; //需將預測值乘回縮放比例, 才是原始比例
          ledcWrite(0, 512 * predictVal + 511); 
          delay(5);
        }     
      }

      // 程式不會走到這裡, 否則一般不用Model後要自行釋放掉Model所占用的記憶體
      // free(parameter_memory);
      // free(memory_ptr);

    } else{
      Serial.println(F("unknown"));
    }
  }
}
