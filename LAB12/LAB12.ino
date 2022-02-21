#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SPIFFS.h>
#include "ModelLite.h"
#include <AsyncWebSocket.h>

//Example select
#define BTN_PIN 12

enum{
  TRAIN_PHASE,
  TEST_PHASE
};
//全域變數
struct{
  ModelLite::DataReader reader;
  ModelLite::Statistics math;
  ModelLite::Model model; 
  ModelLite::MPU6050 mpu6050;
  float *data;
  float *label;
  uint32_t dataLen;
  uint32_t featureInputNum;
  uint32_t labelInputNum;
  uint32_t labelDataLen;
  uint32_t featureDataLen;
  float common_mean;
  float common_std;
  float predictVal;
  uint8_t stage;
}globalVar;

const char* ssid = "Xperia XZ Premium_db49";
const char* password = "ZXC123asd456";
const char* SERVER = "ifttt.com";  // 伺服器網域

const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n"\
"MIIEDzCCAvegAwIBAgIBADANBgkqhkiG9w0BAQUFADBoMQswCQYDVQQGEwJVUzEl\n"\
"MCMGA1UEChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMp\n"\
"U3RhcmZpZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMDQw\n"\
"NjI5MTczOTE2WhcNMzQwNjI5MTczOTE2WjBoMQswCQYDVQQGEwJVUzElMCMGA1UE\n"\
"ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMpU3RhcmZp\n"\
"ZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwggEgMA0GCSqGSIb3\n"\
"DQEBAQUAA4IBDQAwggEIAoIBAQC3Msj+6XGmBIWtDBFk385N78gDGIc/oav7PKaf\n"\
"8MOh2tTYbitTkPskpD6E8J7oX+zlJ0T1KKY/e97gKvDIr1MvnsoFAZMej2YcOadN\n"\
"+lq2cwQlZut3f+dZxkqZJRRU6ybH838Z1TBwj6+wRir/resp7defqgSHo9T5iaU0\n"\
"X9tDkYI22WY8sbi5gv2cOj4QyDvvBmVmepsZGD3/cVE8MC5fvj13c7JdBmzDI1aa\n"\
"K4UmkhynArPkPw2vCHmCuDY96pzTNbO8acr1zJ3o/WSNF4Azbl5KXZnJHoe0nRrA\n"\
"1W4TNSNe35tfPe/W93bC6j67eA0cQmdrBNj41tpvi/JEoAGrAgEDo4HFMIHCMB0G\n"\
"A1UdDgQWBBS/X7fRzt0fhvRbVazc1xDCDqmI5zCBkgYDVR0jBIGKMIGHgBS/X7fR\n"\
"zt0fhvRbVazc1xDCDqmI56FspGowaDELMAkGA1UEBhMCVVMxJTAjBgNVBAoTHFN0\n"\
"YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xMjAwBgNVBAsTKVN0YXJmaWVsZCBD\n"\
"bGFzcyAyIENlcnRpZmljYXRpb24gQXV0aG9yaXR5ggEAMAwGA1UdEwQFMAMBAf8w\n"\
"DQYJKoZIhvcNAQEFBQADggEBAAWdP4id0ckaVaGsafPzWdqbAYcaT1epoXkJKtv3\n"\
"L7IezMdeatiDh6GX70k1PncGQVhiv45YuApnP+yz3SFmH8lU+nLMPUxA2IGvd56D\n"\
"eruix/U0F47ZEUD0/CwqTRV/p2JdLiXTAAsgGh1o+Re49L2L7ShZ3U0WixeDyLJl\n"\
"xy16paq8U4Zt3VekyvggQQto8PT7dL5WXXp59fkdheMtlb71cZBDzI0fmgAKhynp\n"\
"VSJYACPq4xJDKVtHCN2MQWplBqjlIapBtJUhlbl90TSrE9atvNziPTnNvT51cKEY\n"\
"WQPJIrSPnNVeKtelttQKbfi3QBFGmh95DmK/D5fs4C8fF5Q=\n"\
"-----END CERTIFICATE-----\n";

WiFiClientSecure client;           // 宣告HTTPS前端物件
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void showLogo(){
  // Serial.println(F("-------------------------"));
  // Serial.println(F("----------AIfES----------"));         
  // Serial.println(F("-------------------------"));
    
  Serial.println(F(" .----------------.  .----------------.  .----------------.  .----------------.  .----------------. "));
  Serial.println(F("| .--------------. || .--------------. || .--------------. || .--------------. || .--------------. |"));
  Serial.println(F("| |      __      | || |     _____    | || |  _________   | || |  _________   | || |    _______   | |"));
  Serial.println(F("| |     /  \\     | || |    |_   _|   | || | |_   ___  |  | || | |_   ___  |  | || |   /  ___  |  | |"));
  Serial.println(F("| |    / /\\ \\    | || |      | |     | || |   | |_  \\_|  | || |   | |_  \\_|  | || |  |  (__ \\_|  | |"));
  Serial.println(F("| |   / ____ \\   | || |      | |     | || |   |  _|      | || |   |  _|  _   | || |   '.___`-.   | |"));
  Serial.println(F("| | _/ /    \\ \\_ | || |     _| |_    | || |  _| |_       | || |  _| |___/ |  | || |  |`\\____) |  | |"));
  Serial.println(F("| ||____|  |____|| || |    |_____|   | || | |_____|      | || | |_________|  | || |  |_______.'  | |"));
  Serial.println(F("| |              | || |              | || |              | || |              | || |              | |"));
  Serial.println(F("| '--------------' || '--------------' || '--------------' || '--------------' || '--------------' |"));
  Serial.println(F(" '----------------'  '----------------'  '----------------'  '----------------'  '----------------' "));
}

void onSocketEvent(AsyncWebSocket *server,
                   AsyncWebSocketClient *client,
                   AwsEventType type,
                   void *arg,
                   uint8_t *data,
                   size_t len)
{

  switch (type) {
    case WS_EVT_CONNECT:
      printf("來自%s的用戶%u已連線\n", client->remoteIP().toString().c_str(), client->id());
      break;
    case WS_EVT_DISCONNECT:
      printf("用戶%u已離線\n", client->id());
      break;
    case WS_EVT_ERROR:
      printf("用戶%u出錯了：%s\n", client->id(), (char *)data);
      break;
    case WS_EVT_DATA:
      printf("用戶%u傳入資料：%s\n", client->id(), (char *)data);
      // const size_t capacity = JSON_OBJECT_SIZE(2) + 20;
      // DynamicJsonDocument doc(capacity);
      // deserializeJson(doc, data);

      // const char *device = doc["LDR"]; // "LED"
      // int val = doc["val"];               // 資料值
      // if (strcmp(device, "LED") == 0)
      // {
      //   printf("PWM: %d\n", val);
      //   ledcWrite(0, 1023 - val); // 輸出PWM
      // }
      break;
  }
}

void notifyClients() {
  const size_t capacity = JSON_OBJECT_SIZE(2);
  DynamicJsonDocument doc(capacity);

  doc["device"] = "LDR";
  doc["val"] = globalVar.predictVal;

  char data[30];
  serializeJson(doc, data);
  ws.textAll(data);
}

void setup() {
  //init uart
  Serial.begin(115200); //115200 baud rate (If necessary, change in the serial monitor)
  while (!Serial);

  //init mpu6050
  globalVar.mpu6050.init();
  while(!globalVar.mpu6050.isReady()); //wait for mpu6050 get ready

  //init gpio
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BTN_PIN,INPUT);
  digitalWrite(LED_BUILTIN, 1);
  
  //logo
  showLogo();

  //clear globalVar
  uint32_t clearSize = (uint32_t)&globalVar.stage - (uint32_t)&globalVar.data + sizeof(globalVar.stage);
  memset(&globalVar.data, 0, clearSize);

  globalVar.reader.debugInfoTypeConfig(ModelLite::INFO_VERBOSE);
  globalVar.reader.config(ModelLite::MCU_SUPPORT_ESP32,"/dataset/gesture_data/one.txt,/dataset/gesture_data/two.txt,/dataset/gesture_data/three.txt\n", ModelLite::MODE_CATEGORICAL);  //注意讀檔案順序分別對應到one-hot encoding
  globalVar.reader.read();

  globalVar.data = globalVar.reader.categoricalData.total.feature;                      //get data
  globalVar.label = globalVar.reader.categoricalData.total.label;                       //get label
  globalVar.dataLen = globalVar.reader.categoricalData.total.dataLen;                   //資料筆數
  globalVar.featureInputNum = globalVar.reader.categoricalData.total.featureInputNum;   //特徵輸入維度
  globalVar.labelInputNum = globalVar.reader.categoricalData.total.labelInputNum;       //label維度
  globalVar.featureDataLen = globalVar.reader.categoricalData.total.featureDataArryLen; //data的陣列長度(以1維陣列來看)
  globalVar.labelDataLen = globalVar.reader.categoricalData.total.labelDataArryLen;     //label的陣列長度(以1維陣列來看)

  // Check dataset
  // for(int j = 0; j < globalVar.reader.categoricalData.total.dataLen; j++){
  //   Serial.print(F("Feature Data :"));
  //   for(int i = 0; i < globalVar.reader.categoricalData.total.featureInputNum; i++){
  //     Serial.print(globalVar.data[i + j * globalVar.reader.categoricalData.total.featureInputNum]);
  //     if(i != globalVar.reader.categoricalData.total.featureInputNum - 1){
  //       Serial.print(F(","));
  //     }
  //   }
  //   Serial.print(F("\t\t"));

  //   Serial.print("Label Data :");
  //   for(int i = 0; i < globalVar.reader.categoricalData.total.labelInputNum; i++){
  //     Serial.print(globalVar.label[i +  j * globalVar.reader.categoricalData.total.labelInputNum]);
  //     if(i != globalVar.reader.categoricalData.total.labelInputNum - 1){
  //       Serial.print(F(","));
  //     }
  //   }
  //   Serial.println(F(""));
  // }
 

 //WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.print("\nIP位址：");
  Serial.println(WiFi.localIP());

  //config async web server
  server.serveStatic("/", SPIFFS, "/www/ifttt/").setDefaultFile("index.html");
  // server.serveStatic("/img", SPIFFS, "/www/img/");
  // server.serveStatic("/favicon.ico", SPIFFS, "/www/favicon.ico");

  // server.on("/LDR", HTTP_GET, [](AsyncWebServerRequest * req) {
  //   if(globalVar.stage != TEST_PHASE)    req->send(200, "text/plain", String("還未訓練完畢"));
  //   else{
  //     req->send(200, "text/plain", String(globalVar.predictVal));
      
  //   }                                
    
  // });
  server.onNotFound([](AsyncWebServerRequest * req) {
    req->send(404, "text/plain", "Not found");
  });

  ws.onEvent(onSocketEvent); // 附加事件處理程式
  server.addHandler(&ws);
  server.begin();
  Serial.println("HTTP伺服器開工了～");

  client.setCACert(root_ca);

  //IMPORTANT
  //AIfES requires random weights for training
  //Here the random seed is generated by the noise of an analog pin
  uint32_t aRead;
  aRead = analogRead(A5);
  srand(aRead);
  Serial.print("Random seed read: "); 
  Serial.println(aRead);
  Serial.println(F(""));

  Serial.println(F("Gesture Categorical Classify training demo"));
  Serial.println(F("Type >training< to start"));
}

void loop() {
  //get data byte from rx que
  while(Serial.available() > 0 ){
    String str = Serial.readString();
    if(str.indexOf("training") > -1)       //Keyword "training"
    {
        // 資料預處理, data使用正規化之標準差化
        // 計算data平均值
        globalVar.math.mean_data_config.dataLen = globalVar.featureDataLen; 
        globalVar.math.mean_data_config.dataType = ModelLite::DATA_TYPE_F32;
        globalVar.math.mean_data_config.paraBuf = globalVar.data;
        globalVar.common_mean = globalVar.math.mean();
        Serial.print("mean: ");
        Serial.println(globalVar.common_mean);

        // 計算data標準差
        globalVar.math.std_data_config.dataLen = globalVar.featureDataLen;
        globalVar.math.std_data_config.stdType = ModelLite::STATISTICS_POPULATION;
        globalVar.math.std_data_config.dataType = ModelLite::DATA_TYPE_F32;
        globalVar.math.std_data_config.paraBuf = globalVar.data;
        globalVar.common_std = globalVar.math.std();
        Serial.print("std: ");
        Serial.println(globalVar.common_std);
        
        // 正規化
        //特徵輸入正規化: 不論何種類型的問題, 都使用標準差法
        for(int j = 0; j < globalVar.featureDataLen; j++){
          globalVar.data[j] = (globalVar.data[j] - globalVar.common_mean) / globalVar.common_std;
        }

        //Label輸入正規化: 分類問題的label不須正規化

        //training data
        float *input_training_data = globalVar.data;
        uint16_t input_training_shape[] = {globalVar.dataLen, globalVar.featureInputNum}; // Definition of the input shape, 第一個是資料筆數, 第二個以後是個軸的資料維度
        
        aitensor_t input_training_tensor;                    // Creation of the input AIfES tensor
        input_training_tensor.dtype = aif32;                 // Definition of the used data type, here float with 32 bits, different ones are available
        input_training_tensor.dim   = 2;                     // Dimensions of the tensor, here 2 dimensions, as specified at input_shape, 這裡input_training_shape是二軸的陣列, 若未來是圖像可能會到3軸
        input_training_tensor.shape = input_training_shape;  // Set the shape of the input_tensor
        input_training_tensor.data  = input_training_data;   // Assign the input_data array to the tensor. It expects a pointer to the array where the data is stored
        
        // Tensor for the target data
        float *target_data = globalVar.label;
        uint16_t target_shape[] = {globalVar.dataLen, globalVar.labelInputNum};     // Definition of the input shape, 第一個是資料筆數, 第二個以後是個軸的資料維度

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
        ModelLite::layerSequence nnStructure[] = {{ModelLite::LAYER_INPUT,  0, ModelLite::ACTIVATION_NONE},    // input layer
                                                  {ModelLite::LAYER_DENSE, 10, ModelLite::ACTIVATION_RELU},    // hidden layer
                                                  {ModelLite::LAYER_DENSE,  3, ModelLite::ACTIVATION_SOFTMAX}, // output layer
                                                  };
        modelTrainPara.layerSeq = nnStructure;
        modelTrainPara.lossFuncType  = ModelLite::LOSS_FUNC_CORSS_ENTROPY;
        modelTrainPara.optimizerPara = {ModelLite::OPTIMIZER_ADAM, 0.001, 300, 20}; //optimizer type, learning rate, epochs, batch-size
        globalVar.model.train(&modelTrainPara);
      
        // ----------------------------------------- Evaluate the trained model --------------------------
        // Do the inference after training
        //---------------------------
        // GESTURE Classify test data
        //---------------------------
        #define CLASS_TOTAL 3 
        #define PERIOD 10
        #define FEATURE_PARA 6
        #define FEATURE_DIM PERIOD * FEATURE_PARA
        enum{
          LED_ON, //active low
          LED_OFF,
        };
        enum{
          BTN_RELEASE,
          BTN_PRESS,
        };

        float test_data[FEATURE_DIM];
        uint32_t testArrayIndex = 0;
        uint32_t finishedCond = 0;
        uint8_t btnState = 0;
        uint32_t lastMeaureTime = 0;
        
        //訓練完畢後進入while(1)
        globalVar.stage = 1;
        while(1){
          if(digitalRead(BTN_PIN) && btnState == BTN_RELEASE){
            digitalWrite(LED_BUILTIN, LED_ON);
            btnState = BTN_PRESS;
          }else{
            if(!digitalRead(BTN_PIN)) {
              digitalWrite(LED_BUILTIN, LED_OFF);
              btnState = BTN_RELEASE;
              finishedCond = 0;
              testArrayIndex = 0;
            }
          }
            
          if(btnState == BTN_PRESS){
            if(millis() - lastMeaureTime > 100){
              globalVar.mpu6050.update();
              if(finishedCond == PERIOD){

                //測試資料預處理
                for(int i = 0; i < sizeof(test_data) / sizeof(test_data[0]) ; i++){
                  test_data[i] -= globalVar.common_mean;
                  test_data[i] /= globalVar.common_std;
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
                output_test_tensor = globalVar.model.predict(&input_test_tensor);

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
                globalVar.predictVal = max_index + 1;
                notifyClients();

                while(digitalRead(BTN_PIN) == BTN_PRESS);

                // Serial.println("\n開始連接伺服器…");
                // if (!client.connect(SERVER, 443)) {
                //   Serial.println("連線失敗～");
                // } else {       
                //   Serial.println("連線成功！");
                //   String https_get = "GET https://maker.ifttt.com/trigger/https_test/with/key/dg9J7YHL0mMuDaaRGs9pNU HTTP/1.1\n"\
                //                     "Host: " + String(SERVER) + "\n" +\
                //                     "Connection: close\n\n";
                                    
                //   client.print(https_get);

                //   while (client.connected()) {
                //     String line = client.readStringUntil('\n');
                //     if (line == "\r") {
                //       Serial.println("收到HTTPS回應");
                //       break;
                //     }
                //   }
                //   // 接收並顯示伺服器的回應
                //   // while (client.available()) {
                //   //   char c = client.read();
                //   //   Serial.write(c);
                //   // }
                //   client.stop();
                // }

              }else{
                test_data[testArrayIndex] = globalVar.mpu6050.data.accX; testArrayIndex++;
                test_data[testArrayIndex] = globalVar.mpu6050.data.accY; testArrayIndex++;
                test_data[testArrayIndex] = globalVar.mpu6050.data.accZ; testArrayIndex++;
                test_data[testArrayIndex] = globalVar.mpu6050.data.gyrX; testArrayIndex++;
                test_data[testArrayIndex] = globalVar.mpu6050.data.gyrY; testArrayIndex++;
                test_data[testArrayIndex] = globalVar.mpu6050.data.gyrZ; testArrayIndex++;
                finishedCond++;
              }
              lastMeaureTime = millis();
            }
          }
        }
     }else{
      Serial.println(F("unknown"));
    }
  }
}
