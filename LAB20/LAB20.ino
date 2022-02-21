#include "ModelLite.h"

#define DEG_TO_RAD 0.017453293F
#define PERIOD 10
#define FEATURE_PARA 6
#define LABEL_DIM 3
#define ROUND 30
#define CLASS_TOTAL 3 
//100 period, 6 para 算1 round;  共30 round, 分1,2,3
#define FEATURE_LEN (PERIOD * FEATURE_PARA * ROUND * CLASS_TOTAL)
//100 period, 1 para 算1 round; 共30 round, 分1,2,3
#define LABEL_LEN (LABEL_DIM * ROUND * CLASS_TOTAL)
#define BTN_PIN 12

uint8_t btnState = 0;
uint8_t finishedCond = 0;
uint32_t featureArrayIndex = 0;
uint32_t labelArrayIndex = 0;
uint32_t testArrayIndex = 0;
float feature_data[FEATURE_LEN]; 
float label_data[LABEL_LEN];     
float test_data[PERIOD * FEATURE_PARA];
uint8_t stage = 0;
float mean = 0;
float std_diff = 0;

ModelLite::Model model;
ModelLite::MPU6050 mpu6050;
ModelLite::TaskManager taskManager;

//blink led
enum{
  LED_ON, //active low
  LED_OFF,
};
ModelLite::Task blinkLed = {
  .active = false, 
  .period = 1000, 
  .callback = [](){
  static uint8_t state = LED_OFF;
  state = !state;
  digitalWrite(LED_BUILTIN, state); 
}};

//read btn val
enum{
  BTN_RELEASE,
  BTN_PRESS,
};
enum{
  NUMBER1_SAMPLE_PHASE,
  NUMBER2_SAMPLE_PHASE,
  NUMBER3_SAMPLE_PHASE,
  TEST_PHASE,
};
ModelLite::Task readBtn = {
  .active = true, 
  .period = 50, 
  .callback = [](){
  switch(btnState){
    case BTN_RELEASE:
      if(digitalRead(BTN_PIN)) {
        digitalWrite(LED_BUILTIN, LED_ON);
        btnState = BTN_PRESS;
      }
      break;
    case BTN_PRESS:
      if(!digitalRead(BTN_PIN)) {
        Serial.println("get a data");
        digitalWrite(LED_BUILTIN, LED_OFF);
        btnState = BTN_RELEASE;
        finishedCond = 0;
        
        switch(stage){
          case NUMBER1_SAMPLE_PHASE:
            for(int k = 0; k < CLASS_TOTAL; k++){
              if(k == NUMBER1_SAMPLE_PHASE) label_data[labelArrayIndex] = 1;
              else                          label_data[labelArrayIndex] = 0;
              labelArrayIndex++;
            }
            if(featureArrayIndex == FEATURE_LEN / CLASS_TOTAL){
              Serial.println("Number1 Sample Done");
              Serial.println("");
              stage++;
            }
            break;
          case NUMBER2_SAMPLE_PHASE:
            for(int k = 0; k < CLASS_TOTAL; k++){
              if(k == NUMBER2_SAMPLE_PHASE) label_data[labelArrayIndex] = 1;
              else                          label_data[labelArrayIndex] = 0;
              labelArrayIndex++;
            }
            if(featureArrayIndex == FEATURE_LEN / CLASS_TOTAL * 2){
              Serial.println("Number2 Feature Sample Done");
              Serial.println("");
              stage++;
            }
            break;
          case NUMBER3_SAMPLE_PHASE:
            for(int k = 0; k < CLASS_TOTAL; k++){
              if(k == NUMBER3_SAMPLE_PHASE) label_data[labelArrayIndex] = 1;
              else                          label_data[labelArrayIndex] = 0;
              labelArrayIndex++;
            }
            if(featureArrayIndex == FEATURE_LEN){
              Serial.println("Number3 Feature Sample Done");
              Serial.println("");

              stage++;
              featureArrayIndex = 0;
              labelArrayIndex = 0;

              //---------------------------------------------------------------------------
              //get sample done, show sample(feature) data for save txt data
              //---------------------------------------------------------------------------
              uint32_t showIndex = 0;
              uint32_t showIndex2 = 0;

              uint32_t showLabelIndex = 0;
              uint32_t showLabelIndex2 = 0;

              Serial.println("one.txt");
              for(uint32_t x = 0; x < ROUND; x++){
                for(uint32_t y = 0; y < PERIOD * FEATURE_PARA; y++){
                  Serial.print(feature_data[x * PERIOD * FEATURE_PARA + y]);
                  if(y < PERIOD * FEATURE_PARA - 1 ) Serial.print(",");
                  showIndex++;
                  showIndex2++;
                }
                Serial.println();
              }
              for(uint32_t x = 0; x < ROUND; x++){
                for(uint32_t z = 0; z < LABEL_DIM; z++){
                  Serial.print(label_data[x * LABEL_DIM + z]);
                  if(z < LABEL_DIM - 1 ) Serial.print(",");
                  showLabelIndex++;
                  showLabelIndex2++;
                }
                Serial.println();
              }
              Serial.println();

              Serial.println("two.txt");
              for(uint32_t x = 0; x < ROUND; x++){
                for(uint32_t y = 0; y < PERIOD * FEATURE_PARA; y++){
                  Serial.print(feature_data[showIndex + x * PERIOD * FEATURE_PARA + y]);
                  if(y < PERIOD * FEATURE_PARA - 1 ) Serial.print(",");
                  showIndex2++;
                }
                Serial.println();
              }
              for(uint32_t x = 0; x < ROUND; x++){
                for(uint32_t z = 0; z < LABEL_DIM; z++){
                  Serial.print(label_data[showLabelIndex + x * LABEL_DIM + z]);
                  if(z < LABEL_DIM - 1 ) Serial.print(",");
                  showLabelIndex2++;
                }
                Serial.println();
              }
              Serial.println();

              Serial.println("three.txt");
              for(uint32_t x = 0; x < ROUND; x++){
                for(uint32_t y = 0; y < PERIOD * FEATURE_PARA; y++){
                  Serial.print(feature_data[showIndex2 + x * PERIOD * FEATURE_PARA + y]);
                  if(y < PERIOD * FEATURE_PARA - 1 ) Serial.print(",");
                }
                Serial.println();
              }
              for(uint32_t x = 0; x < ROUND; x++){
                for(uint32_t z = 0; z < LABEL_DIM; z++){
                  Serial.print(label_data[showLabelIndex2 + x * LABEL_DIM + z]);
                  if(z < LABEL_DIM - 1 ) Serial.print(",");
                }
                Serial.println();
              }
              Serial.println();

              // 資料預處理, data使用正規化之標準差化
              // 計算data平均值
              ModelLite::Statistics math;
              math.mean_data_config.dataLen = FEATURE_LEN; 
              math.mean_data_config.dataType = ModelLite::DATA_TYPE_F32;
              math.mean_data_config.paraBuf = feature_data;
              mean = math.mean();
              Serial.print("mean: ");
              Serial.println(mean);

              // 計算data標準差
              math.std_data_config.dataLen = FEATURE_LEN;
              math.std_data_config.stdType = ModelLite::STATISTICS_POPULATION;
              math.std_data_config.dataType = ModelLite::DATA_TYPE_F32;
              math.std_data_config.paraBuf = feature_data;
              std_diff = math.std();
              Serial.print("std_diff: ");
              Serial.println(std_diff);

              //將計算出來的mean與std_diff套用到feature data
              for(int i = 0; i < FEATURE_LEN  ; i++){
                feature_data[i] -= mean;
                feature_data[i] /= std_diff;
              }
              
              //label data 正規化
              //多元分類的label不用正規化

              //training data
              float *input_training_data = feature_data;
              uint16_t input_training_shape[] = {ROUND * CLASS_TOTAL, PERIOD * FEATURE_PARA}; // Definition of the input shape, 第一個是資料筆數, 第二個以後是個軸的資料維度
              
              aitensor_t input_training_tensor;                    // Creation of the input AIfES tensor
              input_training_tensor.dtype = aif32;                 // Definition of the used data type, here float with 32 bits, different ones are available
              input_training_tensor.dim   = 2;                     // Dimensions of the tensor, here 2 dimensions, as specified at input_shape, 這裡input_training_shape是二軸的陣列, 若未來是圖像可能會到3軸
              input_training_tensor.shape = input_training_shape;  // Set the shape of the input_tensor
              input_training_tensor.data  = input_training_data;   // Assign the input_data array to the tensor. It expects a pointer to the array where the data is stored
              
              // Tensor for the target data
              float *target_data = label_data;
              uint16_t target_shape[] = {ROUND * CLASS_TOTAL, LABEL_DIM};     // Definition of the input shape, 第一個是資料筆數, 第二個以後是個軸的資料維度

              aitensor_t target_training_tensor;                    // Creation of the input AIfES tensor
              target_training_tensor.dtype = aif32;                 // Definition of the used data type, here float with 32 bits, different ones are available
              target_training_tensor.dim = 2;                       // Dimensions of the tensor, here 2 dimensions, as specified at input_shape
              target_training_tensor.shape = target_shape;          // Set the shape of the input_tensor
              target_training_tensor.data = target_data;            // Assign the target_data array to the tensor. It expects a pointer to the array where the data is stored
    
              //Layer definition
              ModelLite::modelTrainParameter modelTrainPara;
              modelTrainPara.input_tensor  = &input_training_tensor;
              modelTrainPara.target_tensor = &target_training_tensor;
              modelTrainPara.layerSize = 3;
              ModelLite::layerSequence nnStructure[] = {{.layerType = ModelLite::LAYER_INPUT, .neurons = 0,  .activationType = ModelLite::ACTIVATION_NONE},    // input layer
                                                        {.layerType = ModelLite::LAYER_DENSE, .neurons = 10, .activationType = ModelLite::ACTIVATION_RELU},    // hidden layer
                                                        {.layerType = ModelLite::LAYER_DENSE, .neurons = 3,  .activationType = ModelLite::ACTIVATION_SOFTMAX}, // output layer
                                                        };
              modelTrainPara.layerSeq = nnStructure;
              modelTrainPara.lossFuncType  = ModelLite::LOSS_FUNC_CORSS_ENTROPY;
              modelTrainPara.optimizerPara = {ModelLite::OPTIMIZER_ADAM, 0.001, 300, 20}; //optimizer type, learning rate, epochs, batch-size
              model.train(&modelTrainPara);
            }
            break;

          case TEST_PHASE:
            // real time predict
            testArrayIndex = 0;

            // 資料預處理, data使用正規化之標準差化
            for(int i = 0; i < sizeof(test_data) / sizeof(test_data[0]) ; i++){
              test_data[i] -= mean;
              test_data[i] /= std_diff;
            }
            float *input_test_data = test_data;
            uint16_t input_test_shape[] = {1, sizeof(test_data) / sizeof(test_data[0])};

            // Creation of the AIfES input Tensor
            aitensor_t input_test_tensor;               // Creation of the input AIfES tensor
            input_test_tensor.dtype = aif32;            // Definition of the used data type, here float with 32 bits, different ones are available
            input_test_tensor.dim   = 2;                // Dimensions of the tensor, here 1 dimensions, as specified at input_shape
            input_test_tensor.shape = input_test_shape; // Set the shape of the input_tensor
            input_test_tensor.data  = input_test_data;  // Assign the input_data array to the tensor. It expects a pointer to the array where the data is stored

            aitensor_t *output_test_tensor;
            output_test_tensor = model.predict(&input_test_tensor);

            float predictVal[CLASS_TOTAL];
            for(int i = 0; i<CLASS_TOTAL; i++){
              predictVal[i] = ((float* ) output_test_tensor->data)[i]; 
            }
            
            // Check result
            Serial.print(F("Calculated output: "));
            for(int i = 0; i<CLASS_TOTAL-1; i++){
              Serial.print(predictVal[i]); Serial.print(", ");  
            }
            Serial.println(predictVal[CLASS_TOTAL-1]);
            Serial.println(F("")); 

            // find index of max value
            float max = predictVal[0];
            uint8_t max_index = 0;
            for(int i = 1; i<CLASS_TOTAL; i++){
              if(predictVal[i]>max){
                max = predictVal[i];
                max_index = i;
              }
            }
            Serial.print("你寫的數字為: ");
            Serial.println(max_index + 1);
            while(digitalRead(BTN_PIN) == BTN_PRESS);
            break;
        }
      }
      break;
  }                     
}};

//read mpu6050
ModelLite::Task readMPU6050 = {
  .active = true, 
  .period = 100, 
  .callback = [](){
  mpu6050.update();
  if(btnState == BTN_PRESS){
    switch(stage){
      case NUMBER1_SAMPLE_PHASE:
      case NUMBER2_SAMPLE_PHASE:
      case NUMBER3_SAMPLE_PHASE:
        //sampling gesture
        if(finishedCond == PERIOD){
          Serial.println("Finished~~~~");
        }else{
          feature_data[featureArrayIndex] = mpu6050.data.accX; featureArrayIndex++;
          feature_data[featureArrayIndex] = mpu6050.data.accY; featureArrayIndex++;
          feature_data[featureArrayIndex] = mpu6050.data.accZ; featureArrayIndex++;
          feature_data[featureArrayIndex] = mpu6050.data.gyrX; featureArrayIndex++;
          feature_data[featureArrayIndex] = mpu6050.data.gyrY; featureArrayIndex++;
          feature_data[featureArrayIndex] = mpu6050.data.gyrZ; featureArrayIndex++;
          finishedCond++;
        }
        break;

      case TEST_PHASE:
        if(finishedCond == PERIOD){
          Serial.println("Finished~~~~");
        }else{
          test_data[testArrayIndex] = mpu6050.data.accX; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.accY; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.accZ; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.gyrX; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.gyrY; testArrayIndex++;
          test_data[testArrayIndex] = mpu6050.data.gyrZ; testArrayIndex++;
          finishedCond++;
        }
        break;
    }
  }
}};

//show mpu6050 info
ModelLite::Task showMPU6050 = {
  .active = false, 
  .period = 500, 
  .callback = [](){
  Serial.print(F("ACC_X: ")); Serial.println(mpu6050.data.accX);
  Serial.print(F("ACC_Y: ")); Serial.println(mpu6050.data.accY);
  Serial.print(F("ACC_Z: ")); Serial.println(mpu6050.data.accZ);
  Serial.print(F("GYR_X: ")); Serial.println(mpu6050.data.gyrX * DEG_TO_RAD);
  Serial.print(F("GYR_Y: ")); Serial.println(mpu6050.data.gyrY * DEG_TO_RAD);
  Serial.print(F("GYR_Z: ")); Serial.println(mpu6050.data.gyrZ * DEG_TO_RAD);
  Serial.print(F("Temperature: ")); Serial.println(mpu6050.data.temperature);
  Serial.println(F(""));
}};

void initMCU(){
  Serial.begin(115200);
  while (!Serial);
  
  mpu6050.init();
  while(!mpu6050.isReady()); //wait for mpu6050 get ready

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BTN_PIN,INPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  //IMPORTANT
  //AIfES requires random weights for training
  //Here the random seed is generated by the noise of an analog pin
  uint32_t aRead;
  aRead = analogRead(A5);
  srand(641);
  Serial.print("Random seed read: "); 
  Serial.println(aRead);
  Serial.println(F(""));
}

void setup(){
  //init peripherals
  initMCU();

  //register tasks
  taskManager.add(&blinkLed);
  taskManager.add(&readBtn);
  taskManager.add(&readMPU6050);
  taskManager.add(&showMPU6050);
}

void loop(){
  taskManager.execute();
}