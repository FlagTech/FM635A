#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <Flag_DataExporter.h>

#define LED_ON  0
#define LED_OFF 1

// 1個週期(PERIOD)取MPU6050的6個參數(SENSOR_PARA)
// 每10個週期(PERIOD)為一筆特徵資料
// 2種分類各取30筆(ROUND)
#define CLASS_TOTAL 3
#define ROUND 50
#define SENSOR_PARA 3
#define FEATURE_DIM SENSOR_PARA
#define FEATURE_LEN (FEATURE_DIM * ROUND * CLASS_TOTAL)

//------------全域變數------------
// 感測器的物件
SparkFun_APDS9960 apds = SparkFun_APDS9960();

// 匯出蒐集資料會用的物件
Flag_DataExporter exporter;

// 蒐集資料會用到的參數
float sensorData[FEATURE_LEN]; 
uint32_t sensorArrayIndex = 0;
uint32_t collectFinishedCond = 0;
bool showStageInfo = false;
//--------------------------------

void setup() {
  // UART設置
  Serial.begin(115200);

  // 初始化APDS9960
  if(apds.init()) Serial.println(F("APDS-9960 初始化完成"));
  else            Serial.println(F("APDS-9960 初始化錯誤"));
  
  // 啟用APDS-9960光傳感器（無中斷）
  if(apds.enableLightSensor(false)) Serial.println(F("光傳感器現在正在運行"));
  else                              Serial.println(F("光傳感器初始化錯誤"));
  
  // 調整接近傳感器增益
  if(!apds.setProximityGain(PGAIN_2X)) Serial.println(F("設置 PGAIN 時出現問題"));
  
  // 啟用APDS-9960接近傳感器（無中斷）
  if(apds.enableProximitySensor(false)) Serial.println(F("接近傳感器現在正在運行"));
  else                                  Serial.println(F("傳感器初始化錯誤"));
  
  // GPIO設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  Serial.println(F("----- 顏色資料蒐集 -----"));
  Serial.println();
}

void loop(){
  // 偵測是否要開始蒐集資料
  uint8_t proximity_data = 0;
  uint16_t red_light = 0,green_light = 0,blue_light = 0, ambient_light = 0;

  if(!apds.readProximity(proximity_data)){
    Serial.println("Error reading proximity value");
  }

  // 當開始蒐集資料的條件達成時, 開始蒐集
  if (proximity_data == 255 && collectFinishedCond != ROUND) {
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LED_ON);

    // 偵測是否要開始蒐集資料
    if(!apds.readAmbientLight(ambient_light)  ||
       !apds.readRedLight(red_light)          ||
       !apds.readGreenLight(green_light)      ||
       !apds.readBlueLight(blue_light)){
      Serial.println("Error reading light values");
    }else{
      // 計算總信號中各種顏色的比例。 它們被標準化為0至1的範圍。
      float sum = red_light + green_light + blue_light;
      float redRatio = 0;
      float greenRatio = 0;
      float blueRatio = 0;
      // 偵測顏色會用到的參數
      if(sum != 0){
        redRatio = red_light / sum;
        greenRatio = green_light / sum;
        blueRatio = blue_light / sum;
      }
     
      sensorData[sensorArrayIndex] = redRatio;   sensorArrayIndex++;
      sensorData[sensorArrayIndex] = greenRatio; sensorArrayIndex++;
      sensorData[sensorArrayIndex] = blueRatio;  sensorArrayIndex++;
      collectFinishedCond++;
      delay(500);

      Serial.println("取得一筆資料");
      if(collectFinishedCond == ROUND) showStageInfo = true;
    }
  } else {
    // 未蒐集資料時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, LED_OFF);
    
    // 每一個階段都會提示該階段蒐集完成的訊息, 並且僅顯示一次
    if(showStageInfo){
      for(int i = 0; i < CLASS_TOTAL; i++){
        if(sensorArrayIndex == FEATURE_LEN / CLASS_TOTAL * (i+1)){
          Serial.print("物件"); Serial.print(i); Serial.println("顏色取樣完成");   
          Serial.println("請在5秒內換移開本次採樣的物件"); 
          collectFinishedCond = 0;
          for(int q = 0; q<5; q++){
            Serial.println(5-q);
            delay(1000);
          }
          showStageInfo = false;
          if(sensorArrayIndex == FEATURE_LEN){
            // 匯出特徵資料字串
            exporter.dataExport(sensorData, FEATURE_DIM, ROUND, CLASS_TOTAL);
            Serial.println("可以將特徵資料字串複製起來並存成TXT檔, 若需要重新蒐集資料請重置ESP32");
            while(1);
          }
          break;
        }
      }
    }
  }
}
