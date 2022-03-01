/*
  手勢記錄 -- 蒐集訓練資料
*/
#include <Flag_MPU6050.h>
#include <Flag_Switch.h>
#include <Flag_DataExporter.h>

#define LED_ON  0
#define LED_OFF 1
#define COLLECT_BTN_PIN 39

// 1個週期(PERIOD)取MPU6050的6個參數(SENSOR_PARA)
// 每10個週期(PERIOD)為一筆特徵資料
// 3種手勢各取30筆(ROUND)
#define CLASS_TOTAL 3 
#define PERIOD 10
#define ROUND 30
#define SENSOR_PARA 6
#define FEATURE_DIM (PERIOD * SENSOR_PARA)
#define FEATURE_LEN (FEATURE_DIM * ROUND * CLASS_TOTAL)

//------------全域變數------------
// 感測器的物件
Flag_MPU6050 mpu6050;
Flag_Switch collectBtn(COLLECT_BTN_PIN, INPUT);

// 匯出蒐集資料會用的物件
Flag_DataExporter exporter;

// 蒐集資料會用到的參數
float sensorData[FEATURE_LEN]; 
uint32_t sensorArrayIndex = 0, lastArrayIndex = 0;
uint32_t collectFinishedCond = 0;
uint32_t lastMeaureTime = 0;
bool showStageInfo = false;
//--------------------------------

void setup(){
  // UART設置
  Serial.begin(115200);
  
  // mpu6050設置
  mpu6050.init();
  while(!mpu6050.isReady());

  // GPIO設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  Serial.println(F("----- 手勢資料蒐集 -----"));
  Serial.println();
}

void loop(){
  // 當按鈕按下時開始蒐集資料   
  if(collectBtn.read()){
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LED_ON);
  
    // 100ms為一個週期來取一次mpu6050資料, 連續取10個週期作為一筆特徵資料, 也就是一秒會取到一筆特徵資料
    if(millis() - lastMeaureTime > 100){
      // mpu6050資料更新  
      mpu6050.update();

      if(collectFinishedCond == PERIOD){
        // 取得一筆特徵資料
        Serial.println("此筆資料蒐集已完成, 可以放開按鈕進行下一筆資料蒐集~");
        showStageInfo = true;
      }else{
        sensorData[sensorArrayIndex] = mpu6050.data.accX; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.accY; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.accZ; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.gyrX; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.gyrY; sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.gyrZ; sensorArrayIndex++;
        collectFinishedCond++;
      }
      lastMeaureTime = millis();
    }
  }else{
    // 未蒐集資料時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, LED_OFF);
    
    // 按鈕放開, 則代表特徵資料要重新蒐集
    collectFinishedCond = 0;
    
    // 若中途手放開按鈕, 則不足以形成一筆特徵資料
    if(sensorArrayIndex % FEATURE_DIM != 0) sensorArrayIndex = lastArrayIndex;
    else                                    lastArrayIndex   = sensorArrayIndex;

    // 每一個階段都會提示該階段蒐集完成的訊息, 並且僅顯示一次
    if(showStageInfo){
      for(int i = 0; i < CLASS_TOTAL; i++){
        if(sensorArrayIndex == FEATURE_LEN / CLASS_TOTAL * (i+1)){
          Serial.print("手勢"); Serial.print(i); Serial.println("取樣完成");
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