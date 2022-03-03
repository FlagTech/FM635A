/*
  跌倒姿勢記錄 -- 蒐集訓練資料
*/
#include <Flag_MPU6050.h>
#include <Flag_DataExporter.h>

#define LED_ON  0
#define LED_OFF 1

// 1個週期(PERIOD)取MPU6050的6個參數(SENSOR_PARA)
// 每10個週期(PERIOD)為一筆特徵資料
// 2種分類各取30筆(ROUND)
#define PERIOD 10
#define ROUND 50
#define SENSOR_PARA 6
#define FEATURE_DIM (PERIOD * SENSOR_PARA)
#define FEATURE_LEN (FEATURE_DIM * ROUND * 2)

//------------全域變數------------
// 感測器的物件
Flag_MPU6050 mpu6050;

// 匯出蒐集資料會用的物件
Flag_DataExporter exporter;

// 蒐集資料會用到的參數
float sensorData[FEATURE_LEN]; 
uint32_t sensorArrayIndex = 0;
uint32_t collectFinishedCond = 0;
uint32_t lastMeaureTime = 0;
bool showStageInfo = false;
bool collect = false;
bool collectStage = 0;
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

  Serial.println(F("----- 跌倒姿勢資料蒐集 -----"));
  Serial.println();
}

void loop(){
  // 偵測是否要開始蒐集資料
  if(millis() - lastMeaureTime > 100 && !collect){
    // mpu6050資料更新  
    mpu6050.update();
    
    // 依據階段不同, 條件就不同
    switch(collectStage){
      case 0: 
        // 非跌倒狀況中, 加速度參數也要拿來做為"是否要開始蒐集資料"的偵測條件
        if(mpu6050.data.gyrX > 150   || mpu6050.data.gyrX < -150  ||
           mpu6050.data.gyrY > 150   || mpu6050.data.gyrY < -150  ||
           mpu6050.data.gyrZ > 150   || mpu6050.data.gyrZ < -150  ||
           mpu6050.data.accX > 0.25  || mpu6050.data.accX < -0.25 ||
           mpu6050.data.accY > -0.75 || mpu6050.data.accY < -1.25 ||
           mpu6050.data.accZ > 0.25  || mpu6050.data.accZ < -0.25 )
        {
          collect = true;
        }
        break;
      case 1:
        // 跌倒狀況中, 主要拿角速度來做為"是否要開始蒐集資料"的偵測條件
        if(mpu6050.data.gyrX > 150 || mpu6050.data.gyrX < -150 ||
           mpu6050.data.gyrY > 150 || mpu6050.data.gyrY < -150 ||
           mpu6050.data.gyrZ > 150 || mpu6050.data.gyrZ < -150 )
        {
          collect = true;
        }
        break;
    }
    lastMeaureTime = millis();
  }
 
  // 當開始蒐集資料的條件達成時, 開始蒐集
  if(collect) {
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LED_ON);

    // 100ms為一個週期來取一次mpu6050資料, 連續取10個週期作為一筆特徵資料, 也就是一秒會取到一筆特徵資料
    if(millis() - lastMeaureTime > 100){
      //mpu6050資料更新  
      mpu6050.update();

      if(collectFinishedCond == PERIOD){
        // 取得一筆特徵資料
        Serial.println("此筆資料蒐集已完成, 可以進行下一筆資料蒐集");
        showStageInfo = true;
        collect = false;
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

    // 代表資料要重新蒐集
    collectFinishedCond = 0;

    // 每一個階段都會提示該階段蒐集完成的訊息, 並且僅顯示一次
    if(showStageInfo){
      for(int i = 0; i < 2; i++){
        if(sensorArrayIndex == FEATURE_LEN / 2 * (i+1)){
          if(i == 0){
            Serial.println("非跌倒資料取樣完成");
            collectStage++;
          }else{
            Serial.println("跌倒資料取樣完成");
          }       
          showStageInfo = false;
          if(sensorArrayIndex == FEATURE_LEN){
            // 匯出特徵資料字串
            exporter.dataExport(sensorData, FEATURE_DIM, ROUND, 2);
            Serial.println("可以將特徵資料字串複製起來並存成TXT檔, 若需要重新蒐集資料請重置ESP32");
            while(1);
          }
          break;
        }
      }
    }
  }
}  