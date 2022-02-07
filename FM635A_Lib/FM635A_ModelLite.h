#ifndef _FM635A_MODELLITE_H
#define _FM635A_MODELLITE_H

#include <Arduino.h>
#include <aifes.h>  // include the AIfES libary
#include <SPIFFS.h>
#include <ArduinoJson.h>

namespace FM635A_ModelLite
{
  //設太多*.txt其實也沒有辦法讀, 目前先限制讀5個檔案
  enum{
    FILE_LIST_MAX_NUM = 5 
  };

  //debug info選項
  enum{
    INFO_VERBOSE,
    INFO_SIMPLE,
    INFO_BASE,
  };

  //MCU選項, 目前僅有實作ESP32
  enum{
    MCU_SUPPORT_ESP32,
    MCU_SUPPORT_ESP8266,
    MCU_SUPPORT_UNO,
    MCU_SUPPORT_NANO,
  };

  //統計物件用的選項
  enum{
    DATA_TYPE_F32,
  };

  //統計物件用的選項
  enum{
    STATISTICS_POPULATION,
    STATISTICS_SAMPLE,
  };
  
  //data reader會用到的選項
  enum{
    MODE_REGRESSION,
    MODE_BINARY,
    MODE_CATEGORICAL,
  };
  
  //activation func會用到的選項
  enum{
    ACTIVATION_RELU,
    ACTIVATION_SIGMOID,
    ACTIVATION_SOFTMAX,
    ACTIVATION_NONE,
  };
  
  //神經層選項
  enum{
    LAYER_INPUT,
    LAYER_DENSE,
  };

  //損失函數選項
  enum{
    LOSS_FUNC_MSE,
    LOSS_FUNC_CORSS_ENTROPY,
  };
    
  //優化器選項
  enum{
    OPTIMIZER_ADAM,
    OPTIMIZER_SGD,
  };
  
  //for modelTrainParameter
  typedef struct {
    uint8_t layerType;
    uint32_t neurons;
    uint8_t activationType;
  }layerSequence;

  //for modelTrainParameter
  typedef struct {
    uint8_t optimizerType;
    float learningRate;
    uint16_t epochs;
    uint32_t batch_size;
  }optimizerParameter;

  //創建神經網路會用的結構, 目的是包成參數結構餵入創建神經網路的API
  typedef struct {
    aitensor_t *input_tensor;  //for feature
    aitensor_t *target_tensor; //for label 
    uint32_t layerSize;        
    layerSequence *layerSeq;
    uint8_t lossFuncType;
    optimizerParameter optimizerPara;
  }modelTrainParameter;

  class Model {
    
    public :
      //constructor
      Model(){
        _debugInfoType = INFO_VERBOSE;
        _heapRecHead = NULL;
        _heapRecCurrent = NULL;
      }
      
      //Funtion Name: debugInfoTypeConfig
      //Purpose: 設定debug訊息種類
      //Parameter:
      //  infoType: 可以選擇INFO_VERBOSE, INFO_SIMPLE, INFO_BASE
      //Return: None
      void debugInfoTypeConfig(uint8_t infoType = INFO_VERBOSE){
        _debugInfoType = infoType;
      }

      //Funtion Name: config
      //Purpose: 設定MCU類型、model.json的檔案路徑
      //Parameter:
      //  mcu_type: 由於檔案系統庫不同, 所以要分MCU
      //  jason_file_path: model.json的檔案路徑
      //Return: None
      void config(uint8_t mcu_type, String jason_file_path){
        //讀取*.json用的字串
        char *jsonFileStr;

        //由於檔案系統庫不同, 所以要分MCU
        if(mcu_type == MCU_SUPPORT_ESP32){
          if(!SPIFFS.begin(true)){
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("無法掛載SPIFFS分區, 請重置"));
            }
            while(1){}  //blocking
          }

          File file = SPIFFS.open(jason_file_path);                   //default read file mode
          jsonFileStr  = (char *) malloc(sizeof(char) * file.size()); //按照file.size動態分配足夠的char出來
          uint32_t tempCnt = 0;

          if(!file){
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("無法打開文件, 請重置"));
            }
            while(1){} //blocking
          }else{
            while(file.available()){
                jsonFileStr[tempCnt] = (char)file.read(); //取得jsonFile string, 要餵給_buildNN()
                tempCnt++;
            }

            if(_debugInfoType <= INFO_VERBOSE){
              Serial.println(F("檔案內容如下:"));
              Serial.println(*jsonFileStr);
            }
            
            file.close();
          }

        }else if(mcu_type == MCU_SUPPORT_ESP8266){
          //TODO
        }else if(mcu_type == MCU_SUPPORT_UNO){
          //TODO
        }else if(mcu_type == MCU_SUPPORT_NANO){
          //TODO
        }
        
        //解析*.json並建構ANN
        _buildNN(jsonFileStr);
      }
      
      //Funtion Name: predict
      //Purpose: 模型預測
      //Parameter:
      //  input_tensor: 輸入張量, 輸入資料要先包成輸入張量
      //Return: output_tensor, AIfES自動運算出的輸出張量
      aitensor_t *predict(aitensor_t *input_tensor){
        return aialgo_forward_model(&_model, input_tensor); // Forward pass (inference)  
      }
    
      //Funtion Name: train
      //Purpose: 訓練model用的
      //Parameter:
      //  modelTrainParameter: 此為一個結構, 使用者須先定義好神經網路的結構以及要餵入的feature data與label data
      //Return: None
      void train(modelTrainParameter *para){
        ailayer_t *x;                                 //Layer object from AIfES, contains the layers
        ailayer_dense_t*  tempDense[para->layerSize]; //為了要記住動態分配出來的dense, 好在後續分配好parameter空間後進行權重的initial

        for(int i = 0; i < para->layerSize; i++){
          if(para->layerSeq[i].layerType == LAYER_INPUT){
            ailayer_input_t *input_layer = (ailayer_input_t *) malloc(sizeof(ailayer_input_t));
            uint16_t *input_layer_shape = (uint16_t *) malloc(sizeof(uint16_t) * para->input_tensor->dim);  //目前應該都是*2, 因為目前沒有CNN 應該不至於處理到多軸的陣列

            if(para->input_tensor->dim == 2){
              input_layer_shape[0] = 1;                            //第一個元素固定為1, 即便你的input tensor餵了很多筆資料, 仍然是1, 這是參考XOR example的
              input_layer_shape[1] = para->input_tensor->shape[1]; //至於第二個元素則是特徵資料的維度
              input_layer->input_dim = para->input_tensor->dim;
              input_layer->input_shape = input_layer_shape;
            }else{
              if(_debugInfoType <= INFO_BASE){
                Serial.println(F("訓練模型Input Tesnsor Dimension設置錯誤, 請重置"));
              }
              while(1){} //blocking
            }
            _model.input_layer = ailayer_input_f32_default(input_layer);

          }else if(para->layerSeq[i].layerType == LAYER_DENSE){
            
            ailayer_dense_t *dense_layer = (ailayer_dense_t *) malloc(sizeof(ailayer_dense_t));
            dense_layer->neurons = para->layerSeq[i].neurons;
            
            if(i == 1){
              x = ailayer_dense_f32_default(dense_layer, _model.input_layer);
            }else{
              x = ailayer_dense_f32_default(dense_layer, x);
            }
        
            //附加activation func
            if(para->layerSeq[i].activationType == ACTIVATION_RELU){
              ailayer_relu_t *relu_layer = (ailayer_relu_t *) malloc(sizeof(ailayer_relu_t));
              x = ailayer_relu_f32_default(relu_layer, x);
            }else if(para->layerSeq[i].activationType == ACTIVATION_SIGMOID){
              ailayer_sigmoid_t *sigmoid_layer = (ailayer_sigmoid_t *) malloc(sizeof(ailayer_sigmoid_t));
              x = ailayer_sigmoid_f32_default(sigmoid_layer, x);
            }else if(para->layerSeq[i].activationType == ACTIVATION_SOFTMAX){
              ailayer_softmax_t *softmax_layer = (ailayer_softmax_t *) malloc(sizeof(ailayer_softmax_t));
              x = ailayer_softmax_f32_default(softmax_layer, x);
            }else{
              if(_debugInfoType <= INFO_BASE){
                Serial.println(F("訓練模型Activation Function設置錯誤, 請重置"));
              }
              while(1){} //blocking
            }

            //若此層為output layer
            if(i == para->layerSize - 1){
              _model.output_layer = x;
            }
            
            //不得在這裡initial 權重參數會出錯, 但要記住動態分配出來的dense, 好在後續分配好parameter空間後進行權重的initial
            tempDense[i] = dense_layer;
            //所以下面2行不可以拿掉註解
            // aimath_f32_default_init_glorot_uniform(&(dense_layer->weights));
            // aimath_f32_default_init_zeros(&(dense_layer->bias));
            
          }else{
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("訓練模型Layer Type設置錯誤, 請重置"));
            }
            while(1){} //blocking
          }
        }

        if(para->lossFuncType == LOSS_FUNC_MSE){
          //MSE常用於回歸問題, 或二元分類問題
          ailoss_mse_t *mse_loss = (ailoss_mse_t *) malloc(sizeof(ailoss_mse_t)); //Loss: mean squared error                 
          _model.loss = ailoss_mse_f32_default(mse_loss, _model.output_layer);

        }else if (para->lossFuncType == LOSS_FUNC_CORSS_ENTROPY){
          //CORSS_ENTROPY常用於多元分類問題
          ailoss_crossentropy_t *crossentropy_loss = (ailoss_crossentropy_t *) malloc(sizeof(ailoss_crossentropy_t)); //Loss: crossentropy        
          _model.loss = ailoss_crossentropy_f32_default(crossentropy_loss, _model.output_layer);

        }else{
          if(_debugInfoType <= INFO_BASE){
              Serial.println(F("訓練模型Loss Function設置錯誤, 請重置"));
          }
          while(1){} //blocking
        }
        
        // Compile the AIfES model
        aialgo_compile_model(&_model); 

        // ------------------------------- Allocate memory for the parameters of the model ------------------------------
        uint32_t parameter_memory_size = aialgo_sizeof_parameter_memory(&_model);
        if(_debugInfoType <= INFO_SIMPLE){
          Serial.print(F("Required memory for parameter (Weights, Bias, ...):"));
          Serial.print(parameter_memory_size);
          Serial.print(F("Byte"));
          Serial.println(F(""));
        }

        // Serial.print(F("分配parameter_memory前的heap size : "));
        // Serial.println(ESP.getFreeHeap());

        void *parameter_memory = malloc(parameter_memory_size * 2);

        // Serial.print(F("分配parameter_memory後的heap size : "));
        // Serial.println(ESP.getFreeHeap());

        // Distribute the memory to the trainable parameters of the model
        aialgo_distribute_parameter_memory(&_model, parameter_memory, parameter_memory_size);

        // ------------------------------- Initialize the parameters ------------------------------
        //這邊才能initial權重, int = 1是因為從first layer是input layer
        for(int i = 1; i < para->layerSize; i++){ 
          aimath_f32_default_init_glorot_uniform(&tempDense[i]->weights);
          aimath_f32_default_init_zeros(&tempDense[i]->bias);
        }
        // Alternative weight initialisation (但一般都使用上述glorot的初始化權重)
        // Random weights in the value range from -2 to +2
        // The value range of the weights was chosen large, so that learning success is not always given ;)
        // float max = 2.0;
        // float min = -2.0;
        // aimath_f32_default_tensor_init_uniform(&dense_layer_1.weights,max,min);
        // aimath_f32_default_tensor_init_uniform(&dense_layer_1.bias,max,min);
        // aimath_f32_default_tensor_init_uniform(&dense_layer_2.weights,max,min);
        // aimath_f32_default_tensor_init_uniform(&dense_layer_2.bias,max,min);
        // aimath_f32_default_tensor_init_uniform(&dense_layer_3.weights,max,min);
        // aimath_f32_default_tensor_init_uniform(&dense_layer_3.bias,max,min);
        // aimath_f32_default_tensor_init_uniform(&dense_layer_4.weights,max,min);
        // aimath_f32_default_tensor_init_uniform(&dense_layer_4.bias,max,min);

        // -------------------------------- Define the optimizer for training ---------------------
        aiopti_t *optimizer = (aiopti_t *) malloc(sizeof(aiopti_t)); // Object for the optimizer

        if(para->optimizerPara.optimizerType == OPTIMIZER_ADAM){
          //ADAM optimizer (普遍都使用ADAM), 底下參數都是典型值, 有需要瞭解可以參考旗標出版的tf.keras一書
          aiopti_adam_f32_t *adam_opti = (aiopti_adam_f32_t *) malloc(sizeof(aiopti_adam_f32_t)); 
          adam_opti->learning_rate = para->optimizerPara.learningRate;
          adam_opti->beta1 = 0.9f;
          adam_opti->beta2 = 0.999f;
          adam_opti->eps = 1e-7;

          optimizer = aiopti_adam_f32_default(adam_opti);
        }else if(para->optimizerPara.optimizerType == OPTIMIZER_SGD){
          // Alternative: SGD Gradient descent optimizer (SGD是最基本的優化器, 所有優化器都是基於此而衍伸出來的)
          aiopti_sgd_f32_t *sgd_opti = (aiopti_sgd_f32_t *) malloc(sizeof(aiopti_sgd_f32_t)); 
          sgd_opti->learning_rate = para->optimizerPara.learningRate;
          sgd_opti->momentum = 0.0f;

          optimizer = aiopti_sgd_f32_default(sgd_opti);
        }else{
          if(_debugInfoType <= INFO_BASE){
              Serial.println(F("訓練模型Optimizer設置錯誤, 請重置"));
          }
          while(1){} //blocking
        }

        // -------------------------------- Allocate and schedule the working memory for training ---------
        uint32_t memory_size = aialgo_sizeof_training_memory(&_model, optimizer);
        if(_debugInfoType <= INFO_SIMPLE){
          //printf("Required memory for the training (Intermediate results, gradients, optimization memory): %d Byte\n", memory_size);
          Serial.print(F("Required memory for the training (Intermediate results, gradients, optimization memory):"));
          Serial.print(memory_size);
          Serial.print(F("Byte"));
          Serial.println(F(""));
        }
        // Serial.print(F("分配training所需的記憶體前的heap size : "));
        // Serial.println(ESP.getFreeHeap());

        void *memory_ptr = malloc(memory_size * 2);

        // Serial.print(F("分配training所需的記憶體後的heap size : "));
        // Serial.println(ESP.getFreeHeap());

        // Schedule the memory over the model
        aialgo_schedule_training_memory(&_model, optimizer, memory_ptr, memory_size);
        
        // Initialize the AIfES model
        aialgo_init_model_for_training(&_model, optimizer);
 
        // ------------------------------------- Run the training ------------------------------------
        float loss;
        uint32_t batch_size = para->optimizerPara.batch_size;  // 資料筆數, 當作batch size
        uint16_t epochs = para->optimizerPara.epochs;          // 學習次數
        uint16_t print_interval = epochs / 10;

        if(_debugInfoType <= INFO_BASE){
          Serial.println(F("Start training"));
        }
        for(int i = 0; i < epochs; i++){
          // One epoch of training. Iterates through the whole data once
          aialgo_train_model(&_model, para->input_tensor, para->target_tensor, optimizer, batch_size);
        
          // Calculate and print loss every print_interval epochs
          if(i % print_interval == 0){
            aialgo_calc_loss_model_f32(&_model, para->input_tensor, para->target_tensor, &loss);
            if(_debugInfoType <= INFO_BASE){
              Serial.print(F("Epoch: "));
              Serial.print(i);
              Serial.print(F(" Loss: "));
              Serial.print(loss);
              Serial.println(F(""));
            }
          }
        }
        if(_debugInfoType <= INFO_BASE){
          Serial.println(F("Finished training"));
        }
      }
      
      void freeMemory(){
        //使用malloc則要記得釋放, 要做一個
        //free(memory_ptr);
        //free(parameter_memory);
        heapRecord *x;
        x = _heapRecHead;
        while(1){
          free(x->pt);
          if(x->next != NULL){
            x = x->next;
          }else{
            break;
          }
        }
      }

    private:
      aimodel_t _model;  // AIfES model
      uint8_t _debugInfoType;
      
      //heap record struct
      typedef struct heapRecord_t{
        void* pt;
        heapRecord_t *next;
      }heapRecord;

      heapRecord *_heapRecHead;
      heapRecord *_heapRecCurrent;
      
      //增加heap rec node
      void _addHeapRecord(heapRecord *rec){
        if(_heapRecHead == NULL){
          _heapRecCurrent = _heapRecHead = rec;
          _heapRecCurrent->next = NULL;
        }else{
          _heapRecCurrent->next = rec;
          _heapRecCurrent = rec;
          _heapRecCurrent->next = NULL;
        }
      }

      //為了讓_buildNN也支援捲積層
      typedef struct weightList_t{
        float *weight;
        weightList_t *next;
      }weightList;

      typedef struct biasList_t{
        float *bias;
        biasList_t *next;
      }biasList;

      typedef struct convLayer_t{
        uint8_t type;
        uint8_t strides;
        uint8_t kernelSize;
        uint8_t kernelTotal;
        uint8_t padding;
        uint8_t activation; 
        
        weightList *weightHead;
        weightList *weightCurrent;
        biasList *biasHead;
        biasList *biasCurrent;

        convLayer_t *next;
      }convLayer;
 
      enum{
        CONV1D,
        CONV2D
      };
// convLayer* conv = (convLayer*) malloc(sizeof(convLayer));
// conv->type = CONV1D;
// conv->stride = 1;
// conv->kernelSize = 3;
// conv->kernelTotal = len(weight[1]); 
// conv->padding = NONE; 
// conv->activation = RELU; 

// //輸入 對 所有kernel的權重(如同之前的 輸入 對 所有神經元的權重)
// //先把所有kernel掃過一維feature
// for(int i = 0; i< conv->kernelTotal; i++){
//   float tmp[conv->kernelSize] = {0};
//   for(int j = 0; j < kernelSize; j++){
//     tmp[j] = weight[j][i]
//   }
//   conv1D(featureData, tmp, kernelSize, strides, padding, activeType);
// }

// void conv1D(){
//   moveStep = len(featureData) - (kernel - 1);
//   float *featureAfterConv = (float *)malloc(moveStep * sizeof(float));
//   for(int i = 0; i< moveStep; i++){ 
//     float sum = 0;
//     for(int j = 0; j<kernelSize; j++){
//       sum += featureData[i*kernelSize + j] * tmp[j];
//     }
//     featureAfterConv[i] = sum;
//   }
// }

      void _buildNN(char *jsonFileStr){
        //get Json object; 這裡doc是local var依據官方說法, 函數返回時, 該doc物件被銷毀時自動會釋放掉所占用的heap, 實測沒問題
        DynamicJsonDocument doc(24576);    //到https://arduinojson.org/v6/assistant/來評估要留多少的RAM空間, 理論上model.json裏頭要包含這個訊息好動態配置記憶體
        deserializeJson(doc, jsonFileStr); 

        if(_debugInfoType <= INFO_SIMPLE){
          Serial.print(F("How much layers of ANN : "));
          Serial.println(doc["layers"].size()); //print出多少個element, 在這裡就是多少層
        }
        
        ailayer_t *x;     // Layer object from AIfES, contains the layers

        //analyze layer by layer
        for(int i = 0; i < doc["layers"].size(); i++){
          const char *layer_class = doc["layers"][i]["class_name"];

          if( _debugInfoType <= INFO_SIMPLE){
            Serial.print(F("Layer")); Serial.print(i + 1);
            Serial.println(" type : " + String(layer_class));
          }

          //轉成String才能用==去比
          if(String(layer_class) == "InputLayer"){ 
            // Input layer
            
            // AIfES的參數不得使用stack, 要存在heap, 所以下2列註解不得使用
            // uint16_t input_layer_shape[] = {1, (uint16_t)doc["layers"][i]["batch_input_shape"][1]}; // Definition of the input layer shape (Must fit to the input tensor)
            // ailayer_input_t input_layer;                                                            // Creation of the AIfES input layer (注意: input_tensor那個是要餵的資料, 不是input layer)
            uint16_t *input_layer_shape  = (uint16_t *) malloc(sizeof(uint16_t) * 2);                  //一個uint16_t是資料數, 另一個uint16_t是資料維度
            ailayer_input_t *input_layer = (ailayer_input_t *) malloc(sizeof(ailayer_input_t));
            
            //add heap rec node
            heapRecord *tmp = (heapRecord *) malloc(sizeof(heapRecord));
            _addHeapRecord(tmp); 
            tmp->pt = (void *)input_layer_shape;

            //add heap rec node
            tmp = (heapRecord *) malloc(sizeof(heapRecord));
            _addHeapRecord(tmp); 
            tmp->pt = (void *)input_layer;

            input_layer_shape[0] = 1; //先這樣設定, 一般只餵一筆資料進行預測, 要餵多筆在主程式用for迴圈去做
            input_layer_shape[1] = (uint16_t)doc["layers"][i]["batch_input_shape"][1]; //輸入數

            input_layer->input_dim = 2;                    // Definition of the input dimension (Must fit to the input tensor) 官方舉例CNN會到2以上
            input_layer->input_shape = input_layer_shape;  // Handover of the input layer shape
            
            _model.input_layer = ailayer_input_f32_default(input_layer);
            
            if( _debugInfoType <= INFO_SIMPLE){
              Serial.print(F("特徵輸入維度: "));
              Serial.println(input_layer_shape[1]);
              Serial.println(F(""));
            }
          }

          else if(String(layer_class) == "Reshape"){
            //TODO
            
          }else if(String(layer_class) == "Conv1D"){
            //TODO
            convLayer* conv = (convLayer*) malloc(sizeof(convLayer));
            const char *padding = doc["layers"][i]["padding"];
            const char *activation = doc["layers"][i]["activation"];
            conv->type = CONV1D;
            conv->strides = doc["layers"][i]["strides"];
            conv->kernelSize = doc["layers"][i]["kernel_size"];

            if(String(padding) == "valid"){
              conv->padding = 0; 
            }else{
              //TODO
              conv->padding = 0; //目前不支援就對了
            }
            
            if(String(activation) == "relu"){
              conv->activation = ACTIVATION_RELU; 
            }else{
              //TODO
              conv->activation = ACTIVATION_RELU;  //目前僅支援RELU
            }
            //conv->kernelTotal = len(weight[1]);  //計算有多少個捲積核
          }
          
          else if(String(layer_class) == "Dense"){
            //hidden layer  or  output layer

            // AIfES的參數不得使用stack, 要存在heap, 所以下列註解不得使用
            //ailayer_dense_t dense_layer; // Creation of the AIfES hidden dense layer
            ailayer_dense_t *dense_layer = (ailayer_dense_t *) malloc(sizeof(ailayer_dense_t));
            
            //add heap rec node
            heapRecord *tmp = (heapRecord *) malloc(sizeof(heapRecord));
            _addHeapRecord(tmp); 
            tmp->pt = (void *)dense_layer;

            //口訣: 第x個輸入對整個神經層的權重
            //get weight : doc["layers"][i]["weights"][0] for weight
            uint32_t weightsTotal =  (uint32_t)doc["layers"][i]["weights"][0].size() * (uint32_t)doc["layers"][i]["units"];  //共幾個輸入 * 共幾個神經元
            float *weights_data  = (float *) malloc(sizeof(float) * weightsTotal);  

            tmp = (heapRecord *) malloc(sizeof(heapRecord));
            _addHeapRecord(tmp); 
            tmp->pt = (void *)weights_data;

            if(_debugInfoType <= INFO_VERBOSE){
              Serial.printf("Weight in Layer%d :\n", i + 1);
            }

            for(int j = 0; j < doc["layers"][i]["weights"][0].size(); j++){      //遍歷所有輸入
              for(int k = 0; k < doc["layers"][i]["units"]; k++){ //遍歷所有該輸入對所有神經元的weight
                weights_data[ k + j * (uint32_t)doc["layers"][i]["units"] ] = (float)doc["layers"][i]["weights"][0][j][k];    //save weight

                if(_debugInfoType <= INFO_VERBOSE){
                  Serial.println(weights_data[ k + j * (uint32_t)doc["layers"][i]["units"] ], 4);
                }
              }
              if(_debugInfoType <= INFO_VERBOSE){
                Serial.println(F(""));
              }
            }

            //get bias : doc["layers"][i]["weights"][1] for bias
            uint32_t biasTotal =  (uint32_t)doc["layers"][i]["units"];  //共幾個神經元
            float *bias_data  = (float *) malloc(sizeof(float) * biasTotal);  

            tmp = (heapRecord *) malloc(sizeof(heapRecord));
            _addHeapRecord(tmp); 
            tmp->pt = (void *)bias_data;

            if(_debugInfoType <= INFO_VERBOSE){
              Serial.printf("Bias in Layer%d :\n", i + 1);
            }

            for(int j = 0; j < doc["layers"][i]["units"]; j++){       //遍歷所有神經元
              bias_data[j] = (float)doc["layers"][i]["weights"][1][j]; //save bias
              if(_debugInfoType <= INFO_VERBOSE){
                Serial.println(bias_data[j], 4);
              }
            }
            if(_debugInfoType <= INFO_VERBOSE){
              Serial.println(F(""));
            }

            dense_layer->neurons = (uint32_t)doc["layers"][i]["units"];     // Number of neurons
            dense_layer->weights.data = weights_data;                       // Passing the hidden layer weights
            dense_layer->bias.data = bias_data;                             // Passing the hidden layer bias weights
              
            //組裝Dense層要判斷前一層是不是輸入層
            if(i == 1){
              //Input層的下一層隱藏層
              x = ailayer_dense_f32_default(dense_layer, _model.input_layer);
            }else{
              //之後的隱藏層或是輸出層
              x = ailayer_dense_f32_default(dense_layer, x);
            }
            
            if(_debugInfoType <= INFO_SIMPLE){
              Serial.print(F("神經元個數: "));
              Serial.println(biasTotal);
            }

            //附加Activation Func
            const char *activation_type = doc["layers"][i]["activation"];
            
            if(String(activation_type) == "softmax"){
              if(_debugInfoType <= INFO_SIMPLE){
                Serial.println(F("Activation Function: Softmax"));
                Serial.println(F(""));
              }
              //ailayer_softmax_t activation_func_of_layer;   // AIfES的參數不得使用stack, 要存在heap
              ailayer_softmax_t *activation_func_of_layer = (ailayer_softmax_t *) malloc(sizeof(ailayer_softmax_t));

              tmp = (heapRecord *) malloc(sizeof(heapRecord));
              _addHeapRecord(tmp); 
              tmp->pt = (void *)activation_func_of_layer;

              if(i == doc["layers"].size()-1){
                //output latyer
                _model.output_layer = ailayer_softmax_f32_default(activation_func_of_layer, x);
              }else{
                //hidden layer
                x = ailayer_softmax_f32_default(activation_func_of_layer, x);
              }
            }else if(String(activation_type) == "sigmoid"){
              if(_debugInfoType <= INFO_SIMPLE){
                Serial.println(F("Activation Function: Sigmoid"));
                Serial.println(F(""));
              }
              //ailayer_sigmoid_t activation_func_of_layer;   // AIfES的參數不得使用stack, 要存在heap
              ailayer_sigmoid_t *activation_func_of_layer = (ailayer_sigmoid_t *) malloc(sizeof(ailayer_sigmoid_t));
      
              tmp = (heapRecord *) malloc(sizeof(heapRecord));
              _addHeapRecord(tmp); 
              tmp->pt = (void *)activation_func_of_layer;
              
              if(i == doc["layers"].size()-1){
                //output latyer
                _model.output_layer = ailayer_sigmoid_f32_default(activation_func_of_layer, x);
              }else{
                //hidden layer
                x = ailayer_sigmoid_f32_default(activation_func_of_layer, x);
              }
            }else{
              if(_debugInfoType <= INFO_SIMPLE){
                Serial.println(F("Activation Function: Relu"));
                Serial.println(F(""));
              }
              //ailayer_relu_t activation_func_of_layer;   // AIfES的參數不得使用stack, 要存在heap
              ailayer_relu_t *activation_func_of_layer = (ailayer_relu_t *) malloc(sizeof(ailayer_relu_t));

              tmp = (heapRecord *) malloc(sizeof(heapRecord));
              _addHeapRecord(tmp); 
              tmp->pt = (void *)activation_func_of_layer;
              
              if(i == doc["layers"].size()-1){
                //output latyer
                _model.output_layer = ailayer_relu_f32_default(activation_func_of_layer, x);
              }else{
                //hidden layer
                x = ailayer_relu_f32_default(activation_func_of_layer, x);
              }
            }

          }else{
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("模型分析出錯, 請重置"));
            }
            while(1){} //blocking
          }
        }

        aialgo_compile_model(&_model); // Compile the AIfES model
          
        // -------------------------------- Allocate and schedule the working memory for inference ---------
        // Allocate memory for result and temporal data
        uint32_t memory_size = aialgo_sizeof_inference_memory(&_model);
        if(_debugInfoType <= INFO_SIMPLE){
          Serial.print(F("Required memory for intermediate results: "));
          Serial.print(memory_size);
          Serial.println(F("Byte"));
          Serial.println(F(""));
        }
        void *memory_ptr = malloc(memory_size);

        //add heap rec node
        heapRecord *tmp = (heapRecord *) malloc(sizeof(heapRecord));
        _addHeapRecord(tmp); 
        tmp->pt = (void *)memory_ptr;  
  
        // Here is an alternative if no "malloc" should be used
        // Do not forget to comment out the "free(memory_ptr);" at the end if you use this solution
        // byte memory_ptr[memory_size];
        
        // Schedule the memory over the model
        aialgo_schedule_inference_memory(&_model, memory_ptr, memory_size);

        free(jsonFileStr); //別漏了它
      }
  };

  class Statistics{

    public :
      struct{
        uint32_t dataLen;
        uint8_t dataType;
        void *paraBuf; 
      }mean_data_config;
      
      struct{
        uint32_t dataLen;
        uint8_t dataType;
        uint8_t stdType; 
        void *paraBuf; 
      }std_data_config;

      Statistics(){}

      //平均值算法
      float mean(){
        float sum = 0;
        if(mean_data_config.dataType == DATA_TYPE_F32){ 
          for(int i = 0; i<mean_data_config.dataLen; i++){
            sum+=((float*) mean_data_config.paraBuf)[i];
          }
          sum /= mean_data_config.dataLen; 
        }else{
          //TODO
        }
        return sum;
      }

      //標準差算法
      float std(){
        float std=0;
        if(std_data_config.stdType == STATISTICS_POPULATION){
          //母體標準差
          mean_data_config.dataLen = std_data_config.dataLen;
          mean_data_config.dataType = std_data_config.dataType;
          mean_data_config.paraBuf = std_data_config.paraBuf;
          float Mean = mean();
          float sum = 0;
          if(mean_data_config.dataType == DATA_TYPE_F32){ 
            for(int i = 0; i<std_data_config.dataLen; i++){
              sum += sq( ((float*)std_data_config.paraBuf)[i] - Mean );
            }
          }else{
            //TODO
          }
          sum /= std_data_config.dataLen;
          return sqrt(sum);
        }else{
          //樣本標準差
          mean_data_config.dataLen = std_data_config.dataLen;
          mean_data_config.dataType = std_data_config.dataType;
          float Mean = mean();
          float sum = 0;
          if(mean_data_config.dataType == DATA_TYPE_F32){ 
            for(int i = 0; i<std_data_config.dataLen; i++){
              sum += sq( ((float*)std_data_config.paraBuf)[i] - Mean );
            }
          }else{
            //TODO
          }
          sum /= std_data_config.dataLen - 1;
          return sqrt(sum);
        }
      }
  };

  class DataReader {

    public :
      struct regressionData_t{
        float *feature;
        float *label;
        uint32_t featureInputNum;
        uint32_t labelInputNum;
        uint32_t dataLen;
        uint32_t featureDataArryLen;
        uint32_t labelDataArryLen;
      }regressionData;
      
      struct binaryData_t{
        struct{
          float *feature;
          float *label;
          uint32_t featureInputNum;
          uint32_t labelInputNum;
          uint32_t dataLen;
          uint32_t featureDataArryLen;
          uint32_t labelDataArryLen;
        }total;
      }binaryData;

      struct categoricalResult_t{
        struct{
          float *feature;
          float *label;
          uint32_t featureInputNum;
          uint32_t labelInputNum;
          uint32_t dataLen;
          uint32_t featureDataArryLen;
          uint32_t labelDataArryLen;
        }total;
      }categoricalData;

      //Funtion Name: DataReader
      //Purpose: 建構子
      //Parameter: None
      //Return: None
      DataReader(){
        memset(&regressionData,0,sizeof(regressionData));
        memset(&binaryData,0,sizeof(binaryData));
        memset(&categoricalData,0,sizeof(categoricalData));
        _debugInfoType = INFO_VERBOSE;
      }

      //Funtion Name: debugInfoTypeConfig
      //Purpose: 設定debug訊息種類
      //Parameter:
      //  infoType: 可以選擇INFO_VERBOSE, INFO_SIMPLE, INFO_BASE
      //Return: None
      void debugInfoTypeConfig(uint8_t infoType = INFO_VERBOSE){
        _debugInfoType = infoType;
      }
      
      //Funtion Name: setPath
      //Purpose: 設定資料.txt訊息的檔案路徑
      //Parameter:
      //  file_path: 資料.txt訊息的檔案路徑
      //Return: None
      void setPath(String file_path = "None"){
        _filePath = file_path;
      }

      //Funtion Name: config
      //Purpose: 設定MCU類型、資料.txt的檔案路徑、深度學習問題類型
      //Parameter:
      //  mcu_type: 由於檔案系統庫不同, 所以要分MCU
      //  file_path: 資料.txt訊息的檔案路徑
      //  mode: 深度學習問題類型
      //Return: None
      void config(uint8_t mcu_type = MCU_SUPPORT_ESP32, String filePath = "None", uint8_t mode = MODE_REGRESSION){
        _mcu_type = mcu_type;
        _filePath = filePath; //若是分類問題, 使用,號區隔檔案, 換行為結尾
        _mode = mode;
      }

      //Funtion Name: read
      //Purpose: 依據所設定的路徑讀取資料.txt然後將其存入public成員regressionData, binaryData, categoricalData供user存取
      //Parameter: None
      //Return: None
      void read(){
        //由於檔案系統庫不同, 所以要分MCU
        if(_mcu_type == MCU_SUPPORT_ESP32){
          
          //確認檔案系統是否work
          if(!SPIFFS.begin(true)){
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("無法掛載SPIFFS分區, 請重置"));
            }
            while(1){}  //blocking
          }
          
          //依據不同深度學習問題類型做data read的動作
          if(_mode == MODE_REGRESSION){
            uint32_t readDataLen = 0;
            uint32_t featureNum = 0;
            uint32_t labelNum = 0;
            uint32_t stage = 0;  //0 for feature, 1 for label, 2 for finish

            if(_filePath == "None"){
              if(_debugInfoType <= INFO_BASE){
                Serial.println(F("請先設定檔案路徑"));
                return;
              }
            }else{
              File file = SPIFFS.open(_filePath); // read file
              uint8_t get_once = 0;
              char *fileStr  = (char *) malloc(sizeof(char) * file.size());  //用這樣才不會出事, 使用string會非常佔heap; 另外使用stack, 若資料量太大, 也無法完整存入
              uint32_t tempCnt = 0;

              //先得到檔案字串, featureNum, labelNum, readDataLen
              while(file.available()){
                char chr = (char)file.read();
                fileStr[tempCnt] = chr;
                tempCnt++;

                if(chr == ','){
                  if(stage == 0){
                    //feature stage 
                    if(!get_once) featureNum++;
                  }else if(stage == 1){
                    //label stage
                    if(!get_once) labelNum++;
                  }
                }

                if(chr == ' '){
                  if(!get_once) featureNum++;
                  stage = 1;
                }
                
                if(chr == '\n'){
                  if(!get_once) labelNum++;
                  readDataLen += 1; //REGRESSION以'\n'來區別資料筆數
                  stage = 2;        //finish stage
                  get_once = 1;
                }
              }
              
              //有了featureNum, labelNum, readDataLen就可以進行動態記憶體配置
              regressionData.feature = (float *) malloc(sizeof(float) * readDataLen * featureNum);  
              regressionData.label = (float *) malloc(sizeof(float) * readDataLen * labelNum);

              //底下解析file string的資料並寫到動態記憶體中
              stage = 0;  //0 for feature, 1 for label, 2 for finish
              uint32_t strIndex = 0;
              uint32_t dataIndex = 0;
              uint32_t labelIndex = 0;
              String tempStr="";
              
              for(int i = 0; i < readDataLen; i++){
                while(1){
                  if(fileStr[strIndex] != ' ' && fileStr[strIndex] != '\n' && fileStr[strIndex] != ','){
                    tempStr += fileStr[strIndex];
                    strIndex++;
                    
                    if(fileStr[strIndex] == ',' || fileStr[strIndex] == ' '){
                      if(stage == 0){
                        //表示收齊一筆中的一個feature input
                        regressionData.feature[dataIndex] = tempStr.toFloat();
                        tempStr="";
                        dataIndex++;
                      }else if(stage == 1){
                        //表示收齊一筆中的一個label
                        regressionData.label[labelIndex] = tempStr.toFloat();
                        tempStr="";
                        labelIndex++;
                      }
                      if(fileStr[strIndex] == ' '){
                        stage = 1;
                      }
                      strIndex++;
                    }else if(fileStr[strIndex] == '\n'){
                      //表示收齊一筆中的一個label
                      regressionData.label[labelIndex] = tempStr.toFloat();
                      labelIndex++;

                      //表示收齊一筆data, stage歸0
                      stage = 0;
                      tempStr="";
                      strIndex++;
                      break;
                    }
                  }
                }
              }
              
              //更新regressionData資料結構
              //regressionData.feature = ;
              //regressionData.label = ;
              regressionData.featureInputNum = featureNum;
              regressionData.labelInputNum = labelNum;
              regressionData.dataLen = readDataLen;
              regressionData.featureDataArryLen = dataIndex;
              regressionData.labelDataArryLen = labelIndex;

              if(_debugInfoType <= INFO_SIMPLE){
                Serial.printf("資料總數: %d\n", readDataLen);
                Serial.printf("每筆Feature輸入數量: %d\n", featureNum);
                Serial.printf("每筆Label輸入數量: %d\n", labelNum);
                Serial.printf("Feature Total Len: %d\n", dataIndex);
                Serial.printf("Label Total Len: %d\n\n", labelIndex);
              }

              if(_debugInfoType <= INFO_VERBOSE){
                Serial.println(F("檔案內容如下:"));
                for(int a = 0; a < file.size(); a++){
                  Serial.print(fileStr[a]);
                }
              }

              free(fileStr); //記得一定要把file str給釋放掉
              file.close();

              if(_debugInfoType <= INFO_SIMPLE){
                Serial.print(F("讀檔後剩餘Heap : "));
                Serial.println(ESP.getFreeHeap());
              }
            }
          }else if(_mode == MODE_BINARY){
            uint32_t readDataLen = 0;
            uint32_t featureNum = 0;
            uint32_t labelNum = 0;

            if(_filePath == "None"){
              if(_debugInfoType <= INFO_BASE){
                Serial.println(F("請先設定檔案路徑"));
                return;
              }
            }else{
              //先確定分類數量
              uint32_t classifyNum = 0;
              uint32_t strIndex = 0;
              
              while(1){
                if(_filePath[strIndex] == ','){
                  classifyNum++; //讀到一個檔案, 新增一個分類
                }else if(_filePath[strIndex] == '\n'){
                  classifyNum++; //讀到一個檔案, 新增一個分類
                  break;         //解析完畢
                }
                strIndex++;
              }
              
              //2元分類中, 若發現分類數量(檔案數量)不等於2個則代表設置錯誤
              if(classifyNum != 2){
                if(_debugInfoType <= INFO_BASE){
                  Serial.println(F("檔案路徑設置錯誤 or 深度學習類型設置錯誤, 請重置"));
                }
                while(1){}  //blocking
              }
              
              //準備fileList存檔名
              //String *fileList = (String *)malloc(sizeof(String) * classifyNum); //多元分類才需要這樣做
              String fileList[2];
              String tempStr="";
              strIndex = 0;
              uint8_t fileIndex = 0;

              while(1){
                tempStr += _filePath[strIndex];
                strIndex++;

                if(_filePath[strIndex] == ','){
                  fileList[fileIndex] = tempStr;
                  tempStr = "";
                  fileIndex++;
                  strIndex++;
                }else if(_filePath[strIndex] == '\n'){
                  fileList[fileIndex] = tempStr;
                  tempStr = "";
                  fileIndex++;
                  strIndex++;
                  break;      //解析完畢
                }
              }

              //知道檔名後讀個別的檔案字串, featureNum, labelNum, readDataLen
              for(int i = 0; i < classifyNum; i++){
                File file = SPIFFS.open(fileList[i]); // read file
                readDataLen = 0;
                featureNum = 0;
                labelNum = 0;
                uint8_t getOnce = 0;

                char *fileStr  = (char *) malloc(sizeof(char) * file.size());  

                uint32_t tempCnt = 0;
                while(file.available()){
                  char chr = (char)file.read();
                  fileStr[tempCnt]= chr;
                  tempCnt++;

                  if(chr == ','){
                    if(!getOnce) featureNum++;
                  }

                  if(chr == '\n'){  
                    if(!getOnce) featureNum++; //一行之中只會有feature input 因為二元分類的label是以檔名區分
                    if(!getOnce) labelNum++;   //其實二元分類一行之中固定只有一個labelNum, 其值要不是0就是1
                    readDataLen += 1; //BINARY以'\n'來區別資料筆數
                    getOnce = 1;
                  }
                }
                
                //有了featureNum, labelNum, readDataLen就可以進行動態記憶體配置
                _binaryData.file[i].feature  = (float *) malloc(sizeof(float) * readDataLen * featureNum);  
                _binaryData.file[i].label  = (float *) malloc(sizeof(float) * readDataLen * labelNum);

                //底下解析file string的資料並寫到動態記憶體中
                uint32_t strIndex = 0;
                uint32_t dataIndex = 0;
                uint32_t labelIndex = 0;
                String tempStr="";
            
                for(int j = 0; j < readDataLen; j++){
                  while(1){
                    if(fileStr[strIndex] != '\n' && fileStr[strIndex]!= ','){
                      tempStr += fileStr[strIndex];
                      strIndex++;
                      
                      if(fileStr[strIndex] == ','){
                        //表示收齊一筆中的一個feature input
                        _binaryData.file[i].feature[dataIndex] = tempStr.toFloat();
                        tempStr="";
                        dataIndex++;
                        strIndex++;
                      }else if(fileStr[strIndex]== '\n'){
                        //表示收齊一筆中的一個feature input
                        _binaryData.file[i].feature[dataIndex] = tempStr.toFloat();
                        tempStr="";
                        dataIndex++;

                        //表示收齊一筆中的一個label
                        _binaryData.file[i].label[labelIndex] = i; //label 的值 就是檔案索引
                        labelIndex++;

                        strIndex++;
                        break; //表示收齊一筆data
                      }
                    }
                  }
                }

                //更新_binaryData資料結構
                //_binaryData.file[].feature = ;
                //_binaryData.file[].label = ;
                _binaryData.file[i].featureInputNum = featureNum;
                _binaryData.file[i].labelInputNum = labelNum;
                _binaryData.file[i].dataLen = readDataLen;
                _binaryData.file[i].featureDataArryLen = dataIndex;
                _binaryData.file[i].labelDataArryLen = labelIndex;

                if(_debugInfoType <= INFO_SIMPLE){
                  Serial.printf("----File%d----\n", i+1);
                  Serial.printf("資料總數: %d\n", readDataLen);
                  Serial.printf("每筆Feature輸入數量: %d\n", featureNum);
                  Serial.printf("每筆Label輸入數量: %d\n", labelNum);
                  Serial.printf("Feature Total Len: %d\n", dataIndex);
                  Serial.printf("Label Total Len: %d\n\n", labelIndex);
                }
                
                if(_debugInfoType <= INFO_VERBOSE){
                  Serial.println(F("檔案內容如下:"));
                  for(int a = 0; a<file.size(); a++){
                    Serial.print(fileStr[a]);
                  }
                  Serial.println(F(""));
                }
                
                //label 僅有兩類 使用0與1即可, 所以不用特別做one-hot encode
                free(fileStr);//記得一定要把file str給釋放掉
                file.close();
              } //end of file analyze

              binaryData.total.featureInputNum = featureNum; //file[0] & file[1] must to be equal
              binaryData.total.labelInputNum = labelNum;     //file[0] & file[1] must to be equal
              binaryData.total.dataLen = _binaryData.file[0].dataLen + _binaryData.file[1].dataLen ;
              binaryData.total.featureDataArryLen = _binaryData.file[0].featureDataArryLen + _binaryData.file[1].featureDataArryLen;
              binaryData.total.labelDataArryLen = _binaryData.file[0].labelDataArryLen + _binaryData.file[1].labelDataArryLen;

              // if(_debugInfoType <= INFO_SIMPLE){
              //     Serial.print("剩餘Heap : ");
              //     Serial.println(ESP.getFreeHeap());
              // }
               
              if(binaryData.total.featureDataArryLen * sizeof(float) > heap_caps_get_largest_free_block(MALLOC_CAP_32BIT)){
                if(_debugInfoType <= INFO_BASE){
                  Serial.print(F("目前能分配出最大的動態記憶體區段為"));
                  Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
                  Serial.println(F("B"));
                  Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
                }
                while(1);
              }
              //combine two file to one data set
              if(categoricalData.total.featureDataArryLen % 2 == 0){
                binaryData.total.feature = (float *) calloc(binaryData.total.featureDataArryLen, sizeof(float));
              }else{
                binaryData.total.feature = (float *) calloc(binaryData.total.featureDataArryLen, sizeof(float));
              }

              // if(_debugInfoType <= INFO_SIMPLE){
              //     Serial.print("剩餘Heap : ");
              //     Serial.println(ESP.getFreeHeap());
              // }
              
              binaryData.total.label = (float *) calloc(binaryData.total.labelDataArryLen,  sizeof(float));

              // if(_debugInfoType <= INFO_SIMPLE){
              //     Serial.print("剩餘Heap : ");
              //     Serial.println(ESP.getFreeHeap());
              // }

              if(_debugInfoType <= INFO_SIMPLE){
                Serial.printf("----Total----");
                Serial.printf("資料總數: %d\n", binaryData.total.dataLen);
                Serial.printf("每筆Feature輸入數量: %d\n", binaryData.total.featureInputNum);
                Serial.printf("每筆Label輸入數量: %d\n", binaryData.total.labelInputNum);
                Serial.printf("Feature Total Len: %d\n", binaryData.total.featureDataArryLen);
                Serial.printf("Label Total Len: %d\n\n", binaryData.total.labelDataArryLen);
              }

              //in fact, classifyNum == 2 
              for(int q = 0; q < classifyNum; q++){
                for(int k = 0; k < _binaryData.file[q].featureDataArryLen; k++){
                  binaryData.total.feature[k + q * _binaryData.file[0].featureDataArryLen] = _binaryData.file[q].feature[k];
                }  
                for(int y = 0; y < _binaryData.file[q].labelDataArryLen; y++){
                  binaryData.total.label[y +  q * _binaryData.file[0].labelDataArryLen] = _binaryData.file[q].label[y];
                }
                free(_binaryData.file[q].feature);
                free(_binaryData.file[q].label);
              }

              if(_debugInfoType <= INFO_SIMPLE){
                  Serial.print(F("讀檔後剩餘Heap : "));
                  Serial.println(ESP.getFreeHeap());
              }
              
            } // end of SPIFFS check
            
          }else if(_mode == MODE_CATEGORICAL){
            uint32_t readDataLen = 0;
            uint32_t featureNum = 0;
            uint32_t labelNum = 0;

            if(_filePath == "None"){
              if(_debugInfoType <= INFO_BASE){
                Serial.println(F("請先設定檔案路徑"));
                return;
              }
            }else{
              //先確定分類數量
              uint32_t classifyNum = 0;
              uint32_t strIndex = 0;
              
              while(1){
                if(_filePath[strIndex] == ','){
                  classifyNum++; //讀到一個檔案, 新增一個分類
                }else if(_filePath[strIndex] == '\n'){
                  classifyNum++; //讀到一個檔案, 新增一個分類
                  break;         //解析完畢
                }
                strIndex++;
              }
              
              //多元分類中, 若發現分類數量(檔案數量)<=2個則代表設置錯誤
              if(classifyNum <= 2){
                if(_debugInfoType <= INFO_BASE){
                  Serial.println(F("檔案路徑設置錯誤 or 深度學習類型設置錯誤, 請重置"));
                }
                while(1){}  //blocking
              }
              
              //準備fileList存檔名
              //String *fileList = (String *)malloc(sizeof(String) * classifyNum); //再研究 這樣會當掉
              String fileList[FILE_LIST_MAX_NUM];
              String tempStr="";
              strIndex = 0;
              uint8_t fileIndex = 0;
             
              while(1){
                tempStr += _filePath[strIndex];
                strIndex++;

                if(_filePath[strIndex] == ','){
                  fileList[fileIndex] = tempStr;
                  tempStr = "";
                  fileIndex++;
                  strIndex++;
                }else if(_filePath[strIndex] == '\n'){
                  fileList[fileIndex] = tempStr;
                  tempStr = "";
                  fileIndex++;
                  strIndex++;
                  break;      //解析完畢
                }
              }
               
              //知道檔名後讀個別的檔案字串, featureNum, labelNum, readDataLen
              for(int i = 0; i < classifyNum; i++){
                File file = SPIFFS.open(fileList[i]); // read file
                readDataLen = 0;
                featureNum = 0;
                labelNum = 0;
                uint8_t getOnce = 0;

                char *fileStr  = (char *) malloc(sizeof(char) * file.size());  

                uint32_t tempCnt = 0;
                while(file.available()){
                  char chr = (char)file.read();
                  fileStr[tempCnt]= chr;
                  tempCnt++;

                  if(chr == ','){
                    if(!getOnce) featureNum++;
                  }

                  if(chr == '\n'){  
                    if(!getOnce) featureNum++; //一行之中只會有feature input 因為categorical的label是以檔名區分
                    //if(!getOnce) labelNum++; 
                    readDataLen += 1; //categorical以'\n'來區別資料筆數
                    getOnce = 1;
                  }
                }
                
                //有了featureNum, labelNum, readDataLen就可以進行動態記憶體配置
                _categoricalData.file[i].feature  = (float *) malloc(sizeof(float) * readDataLen * featureNum);  
                //_categoricalData.file[i].label  = (float *) malloc(sizeof(float) * readDataLen * labelNum);
                _categoricalData.file[i].label  = (float *) malloc(sizeof(float) * readDataLen * classifyNum);

                //底下解析file string的資料並寫到動態記憶體中
                uint32_t strIndex = 0;
                uint32_t dataIndex = 0;
                uint32_t labelIndex = 0;
                String tempStr="";
               
                for(int j = 0; j < readDataLen; j++){
                  while(1){
                    if(fileStr[strIndex] != '\n' && fileStr[strIndex]!= ','){
                      tempStr += fileStr[strIndex];
                      strIndex++;
                      
                      if(fileStr[strIndex] == ','){
                        //表示收齊一筆中的一個feature input
                        _categoricalData.file[i].feature[dataIndex] = tempStr.toFloat();
                        tempStr="";
                        dataIndex++;
                        strIndex++;
                      }else if(fileStr[strIndex]== '\n'){
                        //表示收齊一筆中的一個feature input
                        _categoricalData.file[i].feature[dataIndex] = tempStr.toFloat();
                        tempStr="";
                        dataIndex++;

                        //表示收齊一筆中的一個label, 要做one-hot encoding
                        for(int k = 0; k < classifyNum; k++){
                          if(k == i){
                            _categoricalData.file[i].label[labelIndex] = 1;
                          }else{
                            _categoricalData.file[i].label[labelIndex] = 0;
                          }
                          labelIndex++;
                        }

                        strIndex++;
                        break; //表示收齊一筆data
                      }
                    }
                  }
                }
                
                //更新_categoricalData資料結構
                //_categoricalData.file[].feature = ;
                //_categoricalData.file[].label = ;
                _categoricalData.file[i].featureInputNum = featureNum;
                //_categoricalData.file[i].labelInputNum = labelNum; //多元分類不是這樣
                _categoricalData.file[i].labelInputNum = classifyNum;//而是這樣
                _categoricalData.file[i].dataLen = readDataLen;
                _categoricalData.file[i].featureDataArryLen = dataIndex;
                _categoricalData.file[i].labelDataArryLen = labelIndex;

                if(_debugInfoType <= INFO_SIMPLE){
                  Serial.printf("----File%d----\n", i+1);
                  Serial.printf("資料總數: %d\n", readDataLen);
                  Serial.printf("每筆Feature輸入數量: %d\n", featureNum);
                  //Serial.printf("每筆Label輸入數量: %d\n", labelNum);
                  Serial.printf("每筆Label輸入數量: %d\n", classifyNum);
                  Serial.printf("Feature Total Len: %d\n", dataIndex);
                  Serial.printf("Label Total Len: %d\n\n", labelIndex);
                }
                
                if(_debugInfoType <= INFO_VERBOSE){
                  Serial.println(F("檔案內容如下:"));
                  for(int a = 0; a<file.size(); a++){
                    Serial.print(fileStr[a]);
                  }
                }
                
                //label 僅有兩類 使用0與1即可, 所以不用特別做one-hot encode
                free(fileStr);//記得一定要把file str給釋放掉
                file.close();
              } //end of file analyze

              categoricalData.total.featureInputNum = featureNum;   //files must equal
              //categoricalData.total.labelInputNum = labelNum;     //多元分類不是這樣
              categoricalData.total.labelInputNum = classifyNum; 

              categoricalData.total.dataLen = 0;
              categoricalData.total.featureDataArryLen = 0;
              categoricalData.total.labelDataArryLen = 0;
              for(int i = 0; i < classifyNum; i++){
                categoricalData.total.dataLen += _categoricalData.file[i].dataLen;
                categoricalData.total.featureDataArryLen += _categoricalData.file[i].featureDataArryLen;
                categoricalData.total.labelDataArryLen += _categoricalData.file[i].labelDataArryLen;
              }

              // if(_debugInfoType <= INFO_SIMPLE){
              //     Serial.print("剩餘Heap : ");
              //     Serial.println(ESP.getFreeHeap());
              // }
              
              if(categoricalData.total.featureDataArryLen * sizeof(float) > heap_caps_get_largest_free_block(MALLOC_CAP_32BIT)){
                if(_debugInfoType <= INFO_BASE){
                  Serial.print(F("目前能分配出最大的動態記憶體區段為"));
                  Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
                  Serial.println(F("B"));
                  Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
                }
                while(1);
              }
              //combine two file to one data set
              if(categoricalData.total.featureDataArryLen % 4 == 0){
                categoricalData.total.feature = (float *) calloc(categoricalData.total.featureDataArryLen, sizeof(float));
              }else{
                categoricalData.total.feature = (float *) calloc(categoricalData.total.featureDataArryLen, sizeof(float));
              }
              
              // if(_debugInfoType <= INFO_SIMPLE){
              //     Serial.print("剩餘Heap : ");
              //     Serial.println(ESP.getFreeHeap());
              // }
              
              categoricalData.total.label = (float *) calloc(categoricalData.total.labelDataArryLen, sizeof(float));

              // if(_debugInfoType <= INFO_SIMPLE){
              //     Serial.print("剩餘Heap : ");
              //     Serial.println(ESP.getFreeHeap());
              // }

              if(_debugInfoType <= INFO_SIMPLE){
                Serial.printf("----Total----");
                Serial.printf("資料總數: %d\n", categoricalData.total.dataLen);
                Serial.printf("每筆Feature輸入數量: %d\n", categoricalData.total.featureInputNum);
                Serial.printf("每筆Label輸入數量: %d\n", categoricalData.total.labelInputNum);
                Serial.printf("Feature Total Len: %d\n", categoricalData.total.featureDataArryLen);
                Serial.printf("Label Total Len: %d\n\n", categoricalData.total.labelDataArryLen);
              }

              for(int q = 0; q < classifyNum; q++){
                uint32_t featureAryLen = 0;
                uint32_t labelAryLen = 0;
                //accumulate array len
                for(int x = 0; x < q; x++){
                  featureAryLen += _categoricalData.file[q].featureDataArryLen;
                  labelAryLen += _categoricalData.file[q].labelDataArryLen;
                }
                for(int k = 0; k < _categoricalData.file[q].featureDataArryLen; k++){
                  categoricalData.total.feature[k + featureAryLen] = _categoricalData.file[q].feature[k];
                }  
                for(int y = 0; y < _categoricalData.file[q].labelDataArryLen; y++){
                  categoricalData.total.label[y + labelAryLen] = _categoricalData.file[q].label[y];
                }
                free(_categoricalData.file[q].feature);
                free(_categoricalData.file[q].label);
              }

              //Serial.println(categoricalData.total.feature[]); //check array is continue
              if(_debugInfoType <= INFO_SIMPLE){
                  Serial.print(F("讀檔後剩餘Heap : "));
                  Serial.println(ESP.getFreeHeap());
              }
              
            } // end of SPIFFS check

          }else{
            if(_debugInfoType <= INFO_BASE){
                Serial.println(" 深度學習類型設置錯誤, 請重置");
            }
            while(1){}  //blocking
          }
          
        }else if(_mcu_type == MCU_SUPPORT_ESP8266){
          //TODO
        }else if(_mcu_type == MCU_SUPPORT_UNO){
          //TODO
        }else if(_mcu_type == MCU_SUPPORT_NANO){
          //TODO
        }
 
      }
      
      void freeMemory(){
        //使用malloc則要記得釋放
        free(regressionData.feature);  
        free(regressionData.label);
        free(binaryData.total.feature);
        free(binaryData.total.label);
        free(categoricalData.total.feature);
        free(categoricalData.total.label);
      }

    private:
      String _filePath;
      uint8_t _debugInfoType;
      uint8_t _mode;
      uint8_t _mcu_type;

      struct _binaryData_t{
        struct{
          float *feature;
          float *label;
          uint32_t featureInputNum;
          uint32_t labelInputNum;
          uint32_t dataLen;
          uint32_t featureDataArryLen;
          uint32_t labelDataArryLen;
        }file[2];
      }_binaryData;

      struct _categoricalData_t{
        struct{
          float *feature;
          float *label;
          uint32_t featureInputNum;
          uint32_t labelInputNum;
          uint32_t dataLen;
          uint32_t featureDataArryLen;
          uint32_t labelDataArryLen;
        }file[FILE_LIST_MAX_NUM];
      }_categoricalData;
  };

  class SerialHandler{
    public:
      float *buf;
      
      //Funtion Name: SerialHandler
      //Purpose: 建構子
      //Parameter: None
      //Return: None
      SerialHandler(){
        _debugInfoType = INFO_VERBOSE;
      }

      //Funtion Name: debugInfoTypeConfig
      //Purpose: 設定debug訊息種類
      //Parameter:
      //  infoType: 可以選擇INFO_VERBOSE, INFO_SIMPLE, INFO_BASE
      //Return: None
      void debugInfoTypeConfig(uint8_t infoType = INFO_VERBOSE){
        _debugInfoType = infoType;
      }

      //Funtion Name: handle
      //Purpose: 儲存一道使用sep分隔資料, 並使用\n結尾的輸入字串訊息到成員變數的buf中
      //Parameter:
      //  sep: 可以自訂分隔資料的字元
      //  end: 可以自訂以結束字元
      //Return: 資料筆數
      uint32_t handle(char sep = ' ', char end = '\n'){
        while(1){
          if(Serial.available() > 0 ){
            String str = Serial.readStringUntil(end);
            str += end; //因為 Serial.readStringUntil()不會讀到end
          
            String strTemp = "";
            uint32_t strIndex = 0;
            uint32_t inputNum=0;
            
            //先算資料筆數
            while(1){
              strTemp += str[strIndex];
              strIndex++;
              if(str[strIndex] == sep){
                inputNum++;
                strTemp = "";
                strIndex++;
              }else if(str[strIndex] == end){
                inputNum++;
                strTemp = "";
                strIndex++;
                break;
              }
            }

            //動態記憶體配置
            buf = (float *) malloc(sizeof(float) * inputNum);
            
            //分配完後才可以寫到buf
            strTemp = "";
            strIndex = 0;
            inputNum=0;
            while(1){
              strTemp += str[strIndex];
              strIndex++;
              if(str[strIndex] == sep){
                buf[inputNum] = strTemp.toFloat();
                inputNum++;
                strTemp = "";
                strIndex++;
              }else if(str[strIndex] == end){
                buf[inputNum] = strTemp.toFloat();
                inputNum++;
                strTemp = "";
                strIndex++;
                return inputNum;
              }
            }
          }
        }
      }
 
      //Funtion Name: freeMemory
      //Purpose: 釋放buf所指的動態分配的記憶體
      //Parameter: None
      //Return: None
      void freeMemory(){
        free(buf);
      }

    private:
      uint8_t _debugInfoType;
  };

} //end of namespace

#endif
