/*
  手勢紀錄 -- 蒐集訓練資料
*/
#include <Flag_MPU6050.h>
#include <Flag_Switch.h>
#include <Flag_DataExporter.h>

// 1 個週期取 MPU6050 的 6 個參數
// 每 10 個週期為一筆特徵資料
// 3 種手勢各取 30 筆
#define CLASS_TOTAL 3
#define SENSOR_PARA 6
#define PERIOD 10
#define ROUND 30
#define FEATURE_DIM (PERIOD * SENSOR_PARA)

//------------全域變數------------
// 感測器的物件
Flag_MPU6050 mpu6050;
Flag_Switch collectBtn(19);

// 匯出蒐集資料會用的物件
Flag_DataExporter exporter;

// 蒐集資料會用到的參數
float sensorData[FEATURE_DIM * ROUND * CLASS_TOTAL];
uint32_t sensorArrayIdx, lastArrayIdx;
uint32_t collectFinishedCond;
uint32_t lastMeasTime;
uint32_t dataCnt;
uint8_t classCnt;
//--------------------------------

void setup(){
  // 序列埠設置
  Serial.begin(115200);
  
  // MPU6050 初始化
  mpu6050.init();
  while(!mpu6050.isReady());

  // 腳位設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // 清除蒐集資料會用到的參數
  sensorArrayIdx = lastArrayIdx = 0;
  collectFinishedCond = 0;
  lastMeasTime = 0;
  dataCnt = 0;
  classCnt = 0;

  Serial.println(
    "請按著按鈕並做出手勢, 以蒐集手勢特徵\n"
  );
}

void loop(){
  // 當按鈕按下時, 開始蒐集資料   
  if(collectBtn.read()){
    // 蒐集資料時, 點亮內建指示燈
    digitalWrite(LED_BUILTIN, LOW);
  
    // 每 100 毫秒為一個週期來取一次 MPU6050 資料
    if(millis() - lastMeasTime > 100){
      // MPU6050 資料更新
      mpu6050.update();
      sensorData[sensorArrayIdx] = mpu6050.data.accX;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.accY;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.accZ;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.gyrX;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.gyrY;
      sensorArrayIdx++;
      sensorData[sensorArrayIdx] = mpu6050.data.gyrZ;
      sensorArrayIdx++;
      collectFinishedCond++;

      // 連續取 10 個週期作為一筆特徵資料, 
      // 也就是一秒會取到一筆特徵資料
      if(collectFinishedCond == PERIOD){
        // 取得一筆特徵資料
        dataCnt++;
        Serial.print("第 ");
        Serial.print(dataCnt);
        Serial.println(" 筆資料蒐集已完成");

        // 每一個階段都會提示該階段蒐集完成的訊息
        if(dataCnt == ROUND){
          classCnt++;
          Serial.print("手勢"); 
          Serial.print(classCnt); 
          Serial.println("取樣完成\n");
          
          if(classCnt == CLASS_TOTAL){
            // 匯出特徵資料字串
            exporter.dataExport(
              sensorData, 
              FEATURE_DIM, 
              ROUND, 
              CLASS_TOTAL
            );
            Serial.print("蒐集完畢, ");
            Serial.print(
              "可以將特徵資料字串複製起來並存成 txt 檔, "
            );
            Serial.println(
              "若需要重新蒐集資料請重置 ESP32"
            );
            while(1);
          }

          // 要蒐集下一類, 故清 0 
          dataCnt = 0;
          Serial.println(
            "請繼續蒐集下一個手勢的特徵資料\n"
          );
        }
       
        lastArrayIdx = sensorArrayIdx;

        // 要先放開按鈕才能再做資料的蒐集
        while(collectBtn.read());
      }
      lastMeasTime = millis();
    }
  }else{
    // 未蒐集資料時, 熄滅內建指示燈
    digitalWrite(LED_BUILTIN, HIGH);

    // 若蒐集的中途放開按鈕, 則不足以形成一筆特徵資料
    sensorArrayIdx = lastArrayIdx;

    // 按鈕放開, 則代表特徵資料要重新蒐集
    collectFinishedCond = 0;
  }
}