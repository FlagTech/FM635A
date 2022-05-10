/*
  農產品分類 -- 蒐集訓練資料
*/
#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <Flag_DataExporter.h>

// 取 APDS9960 的 3 個參數為一筆特徵資料
// 總共蒐集 50 筆
#define FEATURE_DIM 3
#define ROUND       50

//------------全域變數------------
// 感測器的物件
SparkFun_APDS9960 apds = SparkFun_APDS9960();

// 匯出蒐集資料會用的物件
Flag_DataExporter exporter;

// 蒐集資料會用到的參數
float sensorData[FEATURE_DIM * ROUND * 2]; 
uint32_t sensorArrayIdx = 0;
uint32_t collectFinishedCond = 0;
bool showStageInfo = false;
//--------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);

  // 初始化 APDS9960
  if(apds.init()){
    Serial.println("APDS-9960 初始化完成");
  }else{
    Serial.println("APDS-9960 初始化錯誤");
  }

  // 啟用 APDS-9960 光感測器
  if(apds.enableLightSensor(false)){
    Serial.println("光感測器正在運行");
  }else{
    Serial.println("光感測器初始化錯誤");
  }

  // 啟用 APDS-9960 接近感測器
  if(apds.enableProximitySensor(false)){
    Serial.println("接近感測器正在運行");
  }else{ 
    Serial.println("接近感測器初始化錯誤");
  }
  
  // 腳位設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop(){
  uint8_t proximity_data = 0;
  uint16_t red_light = 0;
  uint16_t green_light = 0;
  uint16_t blue_light = 0;
  uint16_t ambient_light = 0;
  
  if(!apds.readProximity(proximity_data)){
    Serial.println("此次讀取接近值錯誤");
  }

  // 當足夠接近時, 開始蒐集
  if(proximity_data == 255 && 
     collectFinishedCond != ROUND){
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LOW);

    if(!apds.readAmbientLight(ambient_light)  ||
       !apds.readRedLight(red_light)          ||
       !apds.readGreenLight(green_light)      ||
       !apds.readBlueLight(blue_light)){
      Serial.println("讀值錯誤");
    }else{
      // 取得各種顏色的比例
      float sum = red_light + green_light + blue_light;
      float redRatio = 0;
      float greenRatio = 0;
      float blueRatio = 0;
      
      if(sum != 0){
        redRatio = red_light / sum;
        greenRatio = green_light / sum;
        blueRatio = blue_light / sum;
      }
     
      sensorData[sensorArrayIdx] = redRatio;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = greenRatio;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = blueRatio;
      sensorArrayIdx++;
      collectFinishedCond++;
      delay(1000);

      Serial.println("取得一筆資料");
      if(collectFinishedCond == ROUND){
        showStageInfo = true;
      }
    }
  }else{
    // 未蒐集資料時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, HIGH);
    
    // 每一個階段都會提示該階段蒐集完成的訊息
    if(showStageInfo){
      for(int i = 0; i < 2; i++){
        if(sensorArrayIdx == (FEATURE_DIM * ROUND) * (i+1)){
          Serial.print("物件");
          Serial.print(i + 1);
          Serial.println("顏色取樣完成");
          Serial.println("請在五秒內移開本次採樣的物件");
          // 倒數 5 秒
          for(int q = 0; q < 5; q++){
            Serial.println(5 - q);
            delay(1000);
          }
          showStageInfo = false;
          collectFinishedCond = 0;

          if(sensorArrayIdx == (FEATURE_DIM * ROUND * 2)){
            // 匯出特徵資料字串
            exporter.dataExport(
              sensorData, 
              FEATURE_DIM, 
              ROUND, 
              2
            );
            Serial.print("蒐集完畢, ");
            Serial.print(
              "可以將特徵資料字串複製起來並存成 txt 檔, "
            );
            Serial.println(
              "若需要重新蒐集資料請重置ESP32"
            );
            while(1);
          }
          break;
        }
      }
    }
  }
}
