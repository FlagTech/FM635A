#ifndef _FLAG_MODEL_H
#define _FLAG_MODEL_H

#include <Arduino.h>
#include <aifes.h>  // include the AIfES libary
#include <SPIFFS.h>
#include <ArduinoJson.h>

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

//for Flag_ModelParameter
typedef struct {
  uint8_t layerType;
  uint32_t neurons;
  uint8_t activationType;
}Flag_LayerSequence;

//for Flag_ModelParameter
typedef struct {
  uint8_t optimizerType;
  float learningRate;
  uint16_t epochs;
  uint32_t batch_size;
}Flag_OptimizerParameter;

//創建神經網路會用的結構, 目的是包成參數結構餵入創建神經網路的API
typedef struct {
  struct{
    uint8_t dim;
    uint16_t *shape;
  }inputLayerPara;
  uint32_t layerSize;        
  Flag_LayerSequence *layerSeq;
  uint8_t lossFuncType;
  Flag_OptimizerParameter optimizerPara;
}Flag_ModelParameter;

class Flag_Model {
  
  public :
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
      MCU_SUPPRRT_AMEBAD
    };
    
    //激活函數選項
    enum{
      ACTIVATION_RELU,
      ACTIVATION_SIGMOID,
      ACTIVATION_SOFTMAX,
      ACTIVATION_LINEAR,
      ACTIVATION_TANH,
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

    //Flag_Model建構子
    Flag_Model(){
      _debugInfoType = INFO_BASE;
      _heapRecHead = NULL;
      _heapRecCurrent = NULL;
      _denseListHead = NULL;
      _denseListCurrent = NULL;
    }
    
    //Funtion Name: debugInfoTypeConfig
    //Purpose: 設定debug訊息種類
    //Parameter:
    //  infoType: 可以選擇INFO_VERBOSE, INFO_SIMPLE, INFO_BASE
    //Return: None
    void debugInfoTypeConfig(uint8_t infoType = INFO_BASE){
      _debugInfoType = infoType;
    }

    //Funtion Name: begin
    //Purpose: model.json的檔案路徑、設定MCU類型
    //Parameter:
    //  jason_file_path: model.json的檔案路徑
    //  mcu_type: 由於檔案系統庫不同, 所以要分MCU
    //Return: None
    void begin(String jason_file_path, uint8_t mcu_type = MCU_SUPPORT_ESP32){
      //讀取*.json用的字串
      char *jsonFileStr;

      //由於檔案系統庫不同, 所以要分MCU, 預設ESP32
      if(mcu_type == MCU_SUPPORT_ESP32){
        
        //掛載SPIFFS
        if(!SPIFFS.begin(true)){
          if(_debugInfoType <= INFO_BASE){
            Serial.println(F("無法掛載SPIFFS分區, 請重置"));
          }
          while(1){}  //blocking
        }

        //開啟model.json
        File file = SPIFFS.open(jason_file_path);  // 預設為 r mode
        
        if(!file || file.isDirectory()){
          if(_debugInfoType <= INFO_BASE){
            Serial.println(F("無法開啟檔案, 請重置"));
          }
          while(1){}  //blocking
        }
        
        //確定是否有足夠的記憶體可以分配
        if(file.size() * sizeof(char) > heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)){
            if(_debugInfoType <= INFO_BASE){
              Serial.print(F("目前能分配出最大的動態記憶體區段為"));
              Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
              Serial.println(F("B"));
              Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
            }
            while(1);
        }
        
        //按照file.size動態分配足夠的char出來 
        jsonFileStr = (char *) malloc(sizeof(char) * file.size());

        //取得jsonFile string, 要餵給_buildNN()
        uint32_t tempCnt = 0;
        while(file.available()){
          jsonFileStr[tempCnt] = (char)file.read(); 
          tempCnt++;
        }

        //show檔案內容
        if(_debugInfoType <= INFO_VERBOSE){
          Serial.println(F("檔案內容如下:"));
          Serial.println(*jsonFileStr);
        }
        
        //關閉檔案
        file.close();
        
      }else if(mcu_type == MCU_SUPPORT_ESP8266){
        //TODO
      }else if(mcu_type == MCU_SUPPORT_UNO){
        //TODO
      }else if(mcu_type == MCU_SUPPORT_NANO){
        //TODO
      }else if(mcu_type == MCU_SUPPRRT_AMEBAD){
        //TODO
      }
      
      //解析*.json並建構ANN
      _buildNN(jsonFileStr);
      
      //別忘了釋放jsonFileStr所佔的heap
      free(jsonFileStr); 
    }
    
    void begin(Flag_ModelParameter *para){
      //底下亂樹種子不得在model的constructor做, 會出事
      //重要!!! AIfES 需要隨機權重進行訓練
      uint16_t randSeed = analogRead(A5); // 這裡的隨機種子是由類比引腳的雜訊產生亂數種子
      srand(randSeed); 
      rand(); // 依據先前所設定的亂數種子來取亂數
      
      //copy一分到私有變數供給tarin()用的
      memcpy (&_para, para, sizeof(Flag_ModelParameter));

      ailayer_t *x;
      _layerSize = para->layerSize;

      for(int i = 0; i < para->layerSize; i++){

        //創建dense list node供後續save model以及initial weight/bias用
        if(i == 0){
          //第一次創建使用Head node
          _denseListHead = (DneseList * )malloc(sizeof(DneseList));
          _denseListHead->next = _denseListHead->prev = NULL;
          _denseListCurrent = _denseListHead;
        }else{
          //第二次(含)以後創建使用Current node
          DneseList *tmpDenseListNode;
          tmpDenseListNode = _denseListCurrent;
          _denseListCurrent->next = (DneseList *)malloc(sizeof(DneseList));
          _denseListCurrent = _denseListCurrent->next;
          _denseListCurrent->prev = tmpDenseListNode;
          _denseListCurrent->next = NULL;
        }
        
        //創建各layer並加入到model中, 並用DenseList Node去記著所有的layer, 供後續save model以及initial weight/bias用
        if(para->layerSeq[i].layerType == LAYER_INPUT){
          ailayer_input_t *input_layer = (ailayer_input_t *) malloc(sizeof(ailayer_input_t));
          uint16_t *input_layer_shape = (uint16_t *) malloc(sizeof(uint16_t) * para->inputLayerPara.dim);  //目前應該都是*2, 因為目前沒有CNN 應該不至於處理到多軸的陣列
      
          //目前僅支援2軸tnesor
          if(para->inputLayerPara.dim == 2){
            input_layer_shape[0] = 1;                              //第一個元素固定為1, 即便你的input tensor餵了很多筆資料, 仍然是1, 這是參考XOR example的
            input_layer_shape[1] = para->inputLayerPara.shape[1]; //至於第二個元素則是特徵資料的維度
            input_layer->input_dim = para->inputLayerPara.dim;
            input_layer->input_shape = input_layer_shape;
          }else{
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("訓練模型Input Tesnsor Dimension設置錯誤, 請重置"));
            }
            while(1){} //blocking
          }

          //加入input_layer到model
          _model.input_layer = ailayer_input_f32_default(input_layer);
          
          //DenseList 節點設定, 目的是為了之後供save model以及initial weight/bias用
          _denseListCurrent->inputLayer = input_layer;
          _denseListCurrent->layerType = LAYER_INPUT;
          _denseListCurrent->activationType = ACTIVATION_NONE;

        }else if(para->layerSeq[i].layerType == LAYER_DENSE){
          ailayer_dense_t *dense_layer = (ailayer_dense_t *) malloc(sizeof(ailayer_dense_t));
          dense_layer->neurons = para->layerSeq[i].neurons;
          _denseListCurrent->neurons = dense_layer->neurons; //DenseList 節點設定

          //串接各層dense layer
          if(_denseListCurrent->prev->layerType == LAYER_INPUT) x = ailayer_dense_f32_default(dense_layer, _model.input_layer);
          else                                                  x = ailayer_dense_f32_default(dense_layer, x);

          //附加activation func
          if(para->layerSeq[i].activationType == ACTIVATION_RELU){
            ailayer_relu_t *relu_layer = (ailayer_relu_t *) malloc(sizeof(ailayer_relu_t));
            x = ailayer_relu_f32_default(relu_layer, x);
            _denseListCurrent->activationType = ACTIVATION_RELU; //DenseList 節點設定

          }else if(para->layerSeq[i].activationType == ACTIVATION_SIGMOID){
            ailayer_sigmoid_t *sigmoid_layer = (ailayer_sigmoid_t *) malloc(sizeof(ailayer_sigmoid_t));
            x = ailayer_sigmoid_f32_default(sigmoid_layer, x);
            _denseListCurrent->activationType = ACTIVATION_SIGMOID; //DenseList 節點設定

          }else if(para->layerSeq[i].activationType == ACTIVATION_SOFTMAX){
            ailayer_softmax_t *softmax_layer = (ailayer_softmax_t *) malloc(sizeof(ailayer_softmax_t));
            x = ailayer_softmax_f32_default(softmax_layer, x);
            _denseListCurrent->activationType = ACTIVATION_SOFTMAX; //DenseList 節點設定

          }else if(para->layerSeq[i].activationType == ACTIVATION_LINEAR){
            //do nothing
            _denseListCurrent->activationType = ACTIVATION_LINEAR; //DenseList 節點設定

          }else if(para->layerSeq[i].activationType == ACTIVATION_NONE){
            //do nothing
            _denseListCurrent->activationType = ACTIVATION_NONE;   //DenseList 節點設定

          }else if(para->layerSeq[i].activationType == ACTIVATION_TANH){
            ailayer_tanh_t *tanh_layer = (ailayer_tanh_t *) malloc(sizeof(ailayer_tanh_t));
            x = ailayer_tanh_f32_default(tanh_layer, x);
            _denseListCurrent->activationType = ACTIVATION_TANH; //DenseList 節點設定

          }else{
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("訓練模型Activation Function設置錯誤, 請重置"));
            }
            while(1){} //blocking
          }

          //若為最後一層output layer, 將其加入到model
          if(i == para->layerSize - 1){
            _model.output_layer = x;
          }
          
          //不可在這裡initial權重參數, 會出錯; 所以要記住動態分配出來的dense, 好在後續分配好parameter空間後進行權重的initial
          _denseListCurrent->dense = dense_layer;
          _denseListCurrent->layerType = LAYER_DENSE;
          
          //因為不可在這裡initial權重參數, 所以下面2行不可以拿掉註解
          // aimath_f32_default_init_glorot_uniform(&(dense_layer->weights));
          // aimath_f32_default_init_zeros(&(dense_layer->bias));
        
        }else{
          if(_debugInfoType <= INFO_BASE){
            Serial.println(F("訓練模型Layer Type設置錯誤, 請重置"));
          }
          while(1){} //blocking
        }
      }
      
      //加入loss function到model中
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

      // ------------------------------------- 顯示模型結構 ------------------------------------
      // Serial.println(F("-------------- Model structure ---------------"));
      // Serial.println(F("Layer:"));
      // aialgo_print_model_structure(&_model);
      // Serial.print(F("\nLoss: "));
      // aialgo_print_loss_specs(_model.loss);
      // Serial.println(F("\n----------------------------------------------\n"));
      // Serial.println();

      // ------------------------------- Allocate memory for the parameters of the model ------------------------------
      uint32_t parameter_memory_size = aialgo_sizeof_parameter_memory(&_model);
      if(_debugInfoType <= INFO_SIMPLE){
        Serial.print(F("Required memory for parameter (Weights, Bias, ...):"));
        Serial.print(parameter_memory_size);
        Serial.print(F("Byte"));
        Serial.println(F(""));
      }

      byte *parameter_memory = (byte *) malloc(parameter_memory_size);
      if(parameter_memory == 0){
        if(_debugInfoType <= INFO_BASE){
          Serial.println(F("ERROR: Not enough memory (RAM) available for parameter (Weights, Biases)."));
          while(1);
        }
      }

      // Serial.print(F("分配parameter_memory後的heap size : "));
      // Serial.println(ESP.getFreeHeap());

      // Distribute the memory to the trainable parameters of the model
      aialgo_distribute_parameter_memory(&_model, parameter_memory, parameter_memory_size);

      // ------------------------------- Initialize the parameters ------------------------------
      //這邊才能initial權重, DenseList從Head開始遍歷所有layer來做parameter inital
      _denseListCurrent = _denseListHead;
      while(1){
        if(_denseListCurrent->layerType != LAYER_INPUT){
          aimath_f32_default_init_glorot_uniform(&_denseListCurrent->dense->weights);
          aimath_f32_default_init_zeros(&_denseListCurrent->dense->bias);
        }

        if(_denseListCurrent->next != NULL)  _denseListCurrent = _denseListCurrent->next;
        else                                 break;
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
      byte *memory_ptr = (byte *) malloc(memory_size);

      if(memory_ptr == 0){
        Serial.println(F("ERROR: Not enough memory (RAM) available for training! Try to use another optimizer (e.g. SGD) or make your net smaller."));
        while(1);
      }

      // Serial.print(F("分配training所需的記憶體後的heap size : "));
      // Serial.println(ESP.getFreeHeap());

      // Schedule the memory over the model
      aialgo_schedule_training_memory(&_model, optimizer, memory_ptr, memory_size);
      
      // Initialize the AIfES model
      aialgo_init_model_for_training(&_model, optimizer);

      //最後copy一份optimizer給train()用
      _optimizer = optimizer;
    }

    //Funtion Name: predict
    //Purpose: 模型預測
    //Parameter:
    //  feature_tensor: 輸入張量, 輸入資料要先包成輸入張量
    //Return: output_tensor, AIfES自動運算出的輸出張量
    aitensor_t *predict(aitensor_t *feature_tensor){
      return aialgo_forward_model(&_model, feature_tensor); // Forward pass (inference)  
    }
  
    //Funtion Name: train
    //Purpose: 訓練model用的
    //Parameter: None
    //Return: None
    void train(aitensor_t *feature_tensor, aitensor_t *label_tensor){
      Flag_ModelParameter *para = &_para;
      float loss;
      uint32_t batch_size = para->optimizerPara.batch_size;  // 資料筆數, 當作batch size
      uint16_t epochs = para->optimizerPara.epochs;          // 學習次數
      uint16_t print_interval = epochs / 10;

      if(_debugInfoType <= INFO_BASE){
        Serial.println(F("--------- 訓練開始 ---------"));
      }
      for(int i = 0; i < epochs; i++){
        // One epoch of training. Iterates through the whole data once
        aialgo_train_model(&_model, feature_tensor, label_tensor, _optimizer, batch_size);
      
        // Calculate and print loss every print_interval epochs
        if(i % print_interval == 0){
          aialgo_calc_loss_model_f32(&_model, feature_tensor, label_tensor, &loss);
          if(_debugInfoType <= INFO_BASE){
            Serial.print(F("Epoch: "));
            Serial.print(i);
            Serial.print(F("\tLoss: "));
            Serial.println(loss);
          }
        }
      }
      if(_debugInfoType <= INFO_BASE){
        Serial.println(F("--------- 訓練結束 ---------"));
        Serial.println();
      }
    }
    
    //freeMemory未完成
    void freeMemory(){
      //使用malloc則要記得釋放, 要做一個
      //free(memory_ptr);
      //free(parameter_memory);
      HeapRecord *x;
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

    void save(String fileName = "model.json"){
      //依據DenseList從Head開始遍歷所有layer來創建json
      _denseListCurrent = _denseListHead;

      DynamicJsonDocument doc(20000);
      
      int layer = 0;
      while(1){
        if(_denseListCurrent->layerType == LAYER_INPUT){
          doc["layers"][layer]["batch_input_shape"][0] = NULL;
          doc["layers"][layer]["batch_input_shape"][1] = _denseListCurrent->inputLayer->input_shape[1];
          doc["layers"][layer]["class_name"] = "InputLayer";

        }else if (_denseListCurrent->layerType == LAYER_DENSE){
          doc["layers"][layer]["class_name"] = "Dense";
          doc["layers"][layer]["units"] = _denseListCurrent->neurons;

          switch(_denseListCurrent->activationType){
            case ACTIVATION_RELU:
              doc["layers"][layer]["activation"] = "relu";
              break;
            case ACTIVATION_SIGMOID:
              doc["layers"][layer]["activation"] = "sigmoid";
              break;
            case ACTIVATION_SOFTMAX:
              doc["layers"][layer]["activation"] = "softmax";
              break;
            case ACTIVATION_LINEAR:
              doc["layers"][layer]["activation"] = "linear";
              break;
            case ACTIVATION_TANH:
              doc["layers"][layer]["activation"] = "tanh";
              break;
            case ACTIVATION_NONE:
              doc["layers"][layer]["activation"] = "none";
              break;
          }
           
          if(_denseListCurrent->dense->weights.dim == 2){
            for(int i = 0; i < _denseListCurrent->dense->weights.shape[0]; i++) {    //第幾個神經元/輸入
              for(int j = 0; j < _denseListCurrent->dense->weights.shape[1]; j++) {  //該神經元/輸入對到下一層所有神經元的權重
                doc["layers"][layer]["weights"][0][i][j] = ((float *)_denseListCurrent->dense->weights.data)[i*_denseListCurrent->dense->weights.shape[1] + j];
              }
            }
          }
          
          int k = 0;
          if(_denseListCurrent->dense->bias.dim == 2){
            for(int i = 0; i < _denseListCurrent->dense->bias.shape[0]; i++) {    //第幾個神經元/輸入
              for(int j = 0; j < _denseListCurrent->dense->bias.shape[1]; j++) {  //該神經元/輸入對到下一層所有神經元的權重
                doc["layers"][layer]["weights"][1][k] = ((float *)_denseListCurrent->dense->bias.data)[i*_denseListCurrent->dense->bias.shape[1] + j];
                k++;
              }
            }
          }
        }

        if(_denseListCurrent->next != NULL)  {
          _denseListCurrent = _denseListCurrent->next;
          layer++;
        }else{
          break;
        }
      }
      String jsonStr;
      serializeJson(doc, jsonStr);
      Serial.println(jsonStr);
      Serial.println();
    }

  private:
    uint8_t _debugInfoType;
    aimodel_t _model;
    
    typedef struct DneseList_t{
      uint8_t layerType;
      uint8_t activationType;
      uint32_t neurons;
      ailayer_input_t *inputLayer;
      ailayer_dense_t* dense;
      DneseList_t *prev;
      DneseList_t *next;
    }DneseList;

    DneseList *_denseListHead;
    DneseList *_denseListCurrent;

    //忘用變數只是為了global而用
    uint32_t _layerSize;
    Flag_ModelParameter _para;
    aiopti_t *_optimizer;

    //heap record struct
    typedef struct HeapRecord_t{
      void* pt;
      HeapRecord_t *next;
    }HeapRecord;

    HeapRecord *_heapRecHead;
    HeapRecord *_heapRecCurrent;
    
    //增加heap rec node
    void _addHeapRecord(HeapRecord *rec){
      if(_heapRecHead == NULL){
        _heapRecCurrent = _heapRecHead = rec;
        _heapRecCurrent->next = NULL;
      }else{
        _heapRecCurrent->next = rec;
        _heapRecCurrent = rec;
        _heapRecCurrent->next = NULL;
      }
    }

    //-----------------Conv1D會用到的程式段(start)----------------
    //-----------------沒時間完成,不爽可以直接刪掉-----------------
    //為了讓_buildNN也支援捲積層
    // typedef struct weightList_t{
    //   float *weight;
    //   weightList_t *next;
    // }weightList;

    // typedef struct biasList_t{
    //   float *bias;
    //   biasList_t *next;
    // }biasList;

    // typedef struct convLayer_t{
    //   uint8_t type;
    //   uint8_t strides;
    //   uint8_t kernelSize;
    //   uint8_t kernelTotal;
    //   uint8_t padding;
    //   uint8_t activation; 
      
    //   weightList *weightHead;
    //   weightList *weightCurrent;
    //   biasList *biasHead;
    //   biasList *biasCurrent;

    //   convLayer_t *next;
    // }convLayer;

    // enum{
    //   CONV1D,
    //   CONV2D
    // };

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
    //-----------------沒時間完成,不爽可以直接刪掉-----------------
    //------------------Conv1D會用到的程式段(end)-----------------



    void _buildNN(char *jsonFileStr){
      // get Json object; 這裡doc是local var依據官方說法, 函數返回時, 該doc物件被銷毀時自動會釋放掉所占用的heap, 實測沒問題
      DynamicJsonDocument doc(24576);    // 到https://arduinojson.org/v6/assistant/來評估要留多少的RAM空間, 理論上model.json裏頭要包含這個訊息好動態配置記憶體
      deserializeJson(doc, jsonFileStr); // str -> json obj

      // print出多少層
      if(_debugInfoType <= INFO_SIMPLE){
        Serial.print(F("How much layers of ANN : "));
        Serial.println(doc["layers"].size()); 
      }
      
      // 組合model過渡用的
      ailayer_t *x; 

      // 層層解析
      for(int i = 0; i < doc["layers"].size(); i++){
        const char *layer_class = doc["layers"][i]["class_name"];

        // print出該神經層的type
        if( _debugInfoType <= INFO_SIMPLE){
          Serial.print(F("Layer")); Serial.print(i + 1);
          Serial.println(" type : " + String(layer_class));
        }

        //轉成String才能用 == 去比
        if(String(layer_class) == "InputLayer"){ 
          // Input layer
          
          // AIfES的參數不得使用stack, 要存在heap, 所以下2列註解不得使用
          // uint16_t input_layer_shape[] = {1, (uint16_t)doc["layers"][i]["batch_input_shape"][1]}; // Definition of the input layer shape (Must fit to the input tensor)
          // ailayer_input_t input_layer;                                                            // Creation of the AIfES input layer (注意: input_tensor那個是要餵的資料, 不是input layer)
          uint16_t *input_layer_shape  = (uint16_t *) malloc(sizeof(uint16_t) * 2);                  //一個uint16_t是資料數, 另一個uint16_t是資料維度
          ailayer_input_t *input_layer = (ailayer_input_t *) malloc(sizeof(ailayer_input_t));
          
          //add heap rec node
          HeapRecord *tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
          _addHeapRecord(tmp); 
          tmp->pt = (void *)input_layer_shape;

          //add heap rec node
          tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
          _addHeapRecord(tmp); 
          tmp->pt = (void *)input_layer;

          //設定input layer shape
          input_layer_shape[0] = 1;                                                  //第一個元素參考官方範例, 固定為1
          input_layer_shape[1] = (uint16_t)doc["layers"][i]["batch_input_shape"][1]; //第二個元素為train_feature_tensor.shape[1], 即訓練data的dim(維度)

          //設定input layer
          input_layer->input_dim = 2;                    // Definition of the input dimension (Must fit to the input tensor) 官方舉例CNN會到2以上
          input_layer->input_shape = input_layer_shape;  // Handover of the input layer shape
          
          //將input layer加入到model
          _model.input_layer = ailayer_input_f32_default(input_layer);
          
          if( _debugInfoType <= INFO_SIMPLE){
            Serial.print(F("特徵輸入維度: "));
            Serial.println(input_layer_shape[1]);
            Serial.println(F(""));
          }
        }


        //-----------------Conv1D會用到的程式段(start)----------------
        //-----------------沒時間完成,不爽可以直接刪掉-----------------
        // else if(String(layer_class) == "Reshape"){
        //   //TODO
          
        // }else if(String(layer_class) == "Conv1D"){
        //   //TODO
        //   convLayer* conv = (convLayer*) malloc(sizeof(convLayer));
        //   const char *padding = doc["layers"][i]["padding"];
        //   const char *activation = doc["layers"][i]["activation"];
        //   conv->type = CONV1D;
        //   conv->strides = doc["layers"][i]["strides"];
        //   conv->kernelSize = doc["layers"][i]["kernel_size"];

        //   if(String(padding) == "valid"){
        //     conv->padding = 0; 
        //   }else{
        //     //TODO
        //     conv->padding = 0; //目前不支援就對了
        //   }
          
        //   if(String(activation) == "relu"){
        //     conv->activation = ACTIVATION_RELU; 
        //   }else{
        //     //TODO
        //     conv->activation = ACTIVATION_RELU;  //目前僅支援RELU
        //   }
        //   //conv->kernelTotal = len(weight[1]);  //計算有多少個捲積核
        // }
        //-----------------沒時間完成,不爽可以直接刪掉-----------------
        //------------------Conv1D會用到的程式段(end)-----------------
        


        else if(String(layer_class) == "Dense"){
          //Hidden layer  or  Output layer

          // AIfES的參數不得使用stack, 要存在heap, 所以下列註解不得使用
          //ailayer_dense_t dense_layer; // Creation of the AIfES hidden dense layer
          ailayer_dense_t *dense_layer = (ailayer_dense_t *) malloc(sizeof(ailayer_dense_t));
          
          //add heap rec node
          HeapRecord *tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
          _addHeapRecord(tmp); 
          tmp->pt = (void *)dense_layer;

          //口訣: 第x個輸入對整個神經層的權重
          //get weight : doc["layers"][i]["weights"][0] for weight
          uint32_t weightsTotal =  (uint32_t)doc["layers"][i]["weights"][0].size() * (uint32_t)doc["layers"][i]["units"];  //共幾個輸入 * 共幾個神經元
          
          //確定是否有足夠的記憶體可以分配
          if(weightsTotal * sizeof(float) > heap_caps_get_largest_free_block(MALLOC_CAP_32BIT)){
              if(_debugInfoType <= INFO_BASE){
                Serial.print(F("目前能分配出最大的動態記憶體區段為"));
                Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
                Serial.println(F("B"));
                Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
              }
              while(1);
          }

          //分配記憶體給輸入->神經層的weights
          float *weights_data  = (float *) malloc(sizeof(float) * weightsTotal);  

          tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
          _addHeapRecord(tmp); 
          tmp->pt = (void *)weights_data;

          if(_debugInfoType <= INFO_VERBOSE){
            Serial.printf("Weights in Layer%d :\n", i + 1);
          }

          // 儲存該層所有w
          for(int j = 0; j < doc["layers"][i]["weights"][0].size(); j++){ //遍歷所有輸入
            for(int k = 0; k < doc["layers"][i]["units"]; k++){           //遍歷所有該輸入對所有神經元的weight
              weights_data[ k + j * (uint32_t)doc["layers"][i]["units"] ] = (float)doc["layers"][i]["weights"][0][j][k]; //save weight

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

          //確定是否有足夠的記憶體可以分配
          if(biasTotal * sizeof(float) > heap_caps_get_largest_free_block(MALLOC_CAP_32BIT)){
              if(_debugInfoType <= INFO_BASE){
                Serial.print(F("目前能分配出最大的動態記憶體區段為"));
                Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
                Serial.println(F("B"));
                Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
              }
              while(1);
          }

          //分配記憶體給輸入->神經層的biases
          float *bias_data  = (float *) malloc(sizeof(float) * biasTotal);  

          tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
          _addHeapRecord(tmp); 
          tmp->pt = (void *)bias_data;

          if(_debugInfoType <= INFO_VERBOSE){
            Serial.printf("Bias in Layer%d :\n", i + 1);
          }
          
          // 儲存該層所有b
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

            tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
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
    
            tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
            _addHeapRecord(tmp); 
            tmp->pt = (void *)activation_func_of_layer;
            
            if(i == doc["layers"].size()-1){
              //output latyer
              _model.output_layer = ailayer_sigmoid_f32_default(activation_func_of_layer, x);
            }else{
              //hidden layer
              x = ailayer_sigmoid_f32_default(activation_func_of_layer, x);
            }
          }else if(String(activation_type) == "linear"){
            if(_debugInfoType <= INFO_SIMPLE){
              Serial.println(F("Activation Function: Linear"));
              Serial.println(F(""));
            }

            if(i == doc["layers"].size()-1){
              //output latyer
              _model.output_layer = x;
            }else{
              //hidden layer
              //do nothing
            }
          }else if(String(activation_type) == "relu"){
            if(_debugInfoType <= INFO_SIMPLE){
              Serial.println(F("Activation Function: Relu"));
              Serial.println(F(""));
            }
            //ailayer_relu_t activation_func_of_layer;   // AIfES的參數不得使用stack, 要存在heap
            ailayer_relu_t *activation_func_of_layer = (ailayer_relu_t *) malloc(sizeof(ailayer_relu_t));

            tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
            _addHeapRecord(tmp); 
            tmp->pt = (void *)activation_func_of_layer;
            
            if(i == doc["layers"].size()-1){
              //output latyer
              _model.output_layer = ailayer_relu_f32_default(activation_func_of_layer, x);
            }else{
              //hidden layer
              x = ailayer_relu_f32_default(activation_func_of_layer, x);
            }
          }else if(String(activation_type) == "tanh"){
            if(_debugInfoType <= INFO_SIMPLE){
              Serial.println(F("Activation Function: Tanh"));
              Serial.println(F(""));
            }
            //ailayer_tanh_t activation_func_of_layer;   // AIfES的參數不得使用stack, 要存在heap
            ailayer_tanh_t *activation_func_of_layer = (ailayer_tanh_t *) malloc(sizeof(ailayer_tanh_t));

            tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
            _addHeapRecord(tmp); 
            tmp->pt = (void *)activation_func_of_layer;
            
            if(i == doc["layers"].size()-1){
              //output latyer
              _model.output_layer = ailayer_tanh_f32_default(activation_func_of_layer, x);
            }else{
              //hidden layer
              x = ailayer_tanh_f32_default(activation_func_of_layer, x);
            }
          }else{
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("模型激活函數分析出錯, 請重置"));
            }
            while(1){} //blocking
          }

        }else{
          if(_debugInfoType <= INFO_BASE){
            Serial.println(F("模型神經層分析出錯, 請重置"));
          }
          while(1){} //blocking
        }
      }

      aialgo_compile_model(&_model); // Compile the AIfES model

      // ------------------------------------- Print the model structure ------------------------------------
      // if(_debugInfoType <= INFO_SIMPLE){
      //   Serial.println(F("-------------- Model structure ---------------"));
      //   aialgo_print_model_structure(&_model);
      //   Serial.println(F("----------------------------------------------\n"));
      // }

      // -------------------------------- Allocate and schedule the working memory for inference ---------
      // Allocate memory for result and temporal data
      uint32_t memory_size = aialgo_sizeof_inference_memory(&_model);
      if(_debugInfoType <= INFO_SIMPLE){
        Serial.print(F("Required memory for intermediate results: "));
        Serial.print(memory_size);
        Serial.println(F("Byte"));
        Serial.println(F(""));
      }
      
      //確定是否有足夠的記憶體可以分配
      if(memory_size > heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)){
          if(_debugInfoType <= INFO_BASE){
            Serial.print(F("目前能分配出最大的動態記憶體區段為"));
            Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
            Serial.println(F("B"));
            Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
          }
          while(1);
      }
      
      //實際分配記憶體
      void *memory_ptr = malloc(memory_size);

      //add heap rec node
      HeapRecord *tmp = (HeapRecord *) malloc(sizeof(HeapRecord));
      _addHeapRecord(tmp); 
      tmp->pt = (void *)memory_ptr;  

      // Here is an alternative if no "malloc" should be used
      // Do not forget to comment out the "free(memory_ptr);" at the end if you use this solution
      // byte memory_ptr[memory_size];
      
      // Schedule the memory over the model
      aialgo_schedule_inference_memory(&_model, memory_ptr, memory_size);
    }
};

#endif
