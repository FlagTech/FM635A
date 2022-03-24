/*
  手勢記錄 -- 蒐集訓練資料
*/
#include <Flag_MPU6050.h>
#include <Flag_Switch.h>
#include <Flag_DataExporter.h>

// 1 個週期 (PERIOD) 取 MPU6050 的 6 個參數 (SENSOR_PARA)
// 每 10 個週期 (PERIOD) 為一筆特徵資料
// 3 種手勢各取 30 筆 (ROUND)
#define CLASS_TOTAL 3 
#define SENSOR_PARA 6
#define PERIOD 10
#define ROUND 5
#define FEATURE_DIM (PERIOD * SENSOR_PARA)
#define FEATURE_LEN (FEATURE_DIM * ROUND * CLASS_TOTAL)

//------------全域變數------------
// 感測器的物件
Flag_MPU6050 mpu6050;
Flag_Switch collectBtn(39);

// 匯出蒐集資料會用的物件
Flag_DataExporter exporter;

// 蒐集資料會用到的參數
float sensorData[FEATURE_LEN]; 
uint32_t sensorArrayIndex = 0, lastArrayIndex = 0;
uint32_t collectFinishedCond = 0;
uint32_t lastMeaureTime = 0;
uint32_t dataCnt = 0;
//--------------------------------

void setup(){
  // 序列埠設置
  Serial.begin(115200);
  
  // MPU6050 設置
  mpu6050.init();
  while(!mpu6050.isReady());

  // 腳位設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println(F("----- 手勢資料蒐集 -----"));
  Serial.println();
}

void loop(){
  // 當按鈕按下時開始蒐集資料   
  if(collectBtn.read()){
    // 蒐集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LOW);
  
    // 每 100 毫秒為一個週期來取一次 MPU6050 資料
    if(millis() - lastMeaureTime > 100){
      // MPU6050 資料更新  
      mpu6050.update();

      // 連續取 10 個週期作為一筆特徵資料, 也就是一秒會取到一筆特徵資料
      if(collectFinishedCond == PERIOD){
        // 取得一筆特徵資料
        dataCnt++;
        Serial.print("第");
        Serial.print(dataCnt);
        Serial.println("筆資料蒐集已完成");

        // 每一種分類資料蒐集完都會提示該階段已蒐集完成的訊息
        for(int i = 0; i < CLASS_TOTAL; i++){
          if(sensorArrayIndex == FEATURE_LEN / CLASS_TOTAL * (i+1)){
            Serial.print("手勢"); 
            Serial.print(i+1); 
            Serial.println("取樣完成");
            dataCnt = 0;
            if(sensorArrayIndex == FEATURE_LEN){
              // 匯出特徵資料字串
              exporter.dataExport(
                sensorData, 
                FEATURE_DIM, 
                ROUND, 
                CLASS_TOTAL
              );
              Serial.println(
                "請將特徵資料字串複製起來並存成txt檔"
              );
              while(1);
            }
            break;
          }
        }

        // 放開按鈕進行下一筆資料收集
        while(collectBtn.read());
      }else{
        sensorData[sensorArrayIndex] = mpu6050.data.accX; 
        sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.accY; 
        sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.accZ; 
        sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.gyrX; 
        sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.gyrY; 
        sensorArrayIndex++;
        sensorData[sensorArrayIndex] = mpu6050.data.gyrZ; 
        sensorArrayIndex++;
        collectFinishedCond++;
      }
      lastMeaureTime = millis();
    }
  }else{
    // 未蒐集資料時, 內建指示燈不亮
    digitalWrite(LED_BUILTIN, HIGH);
    
    // 按鈕放開, 則代表特徵資料要重新蒐集
    collectFinishedCond = 0;
    
    // 若中途手放開按鈕, 則不足以形成一筆特徵資料
    if(sensorArrayIndex % FEATURE_DIM != 0){
      sensorArrayIndex = lastArrayIndex;
    }else{
      lastArrayIndex = sensorArrayIndex;
    }
  }
}