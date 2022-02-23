/*
  跌倒姿勢記錄 -- 蒐集訓練資料
*/
#include <Flag_MPU6050.h>
#include <Flag_Switch.h>

#define LED_ON  0
#define LED_OFF 1
#define BTN_PIN 12

//1 period取6個para, 每10 period為一筆feature data;  fall/others各取30筆
#define PERIOD 10
#define ROUND 30
#define SENSOR_PARA 6
#define FEATURE_DIM (PERIOD * SENSOR_PARA)
#define FEATURE_LEN (FEATURE_DIM * ROUND * 2)

//------------全域變數------------
Flag_MPU6050 mpu6050;
Flag_Switch btn(BTN_PIN, INPUT);

float sensor_data[FEATURE_LEN]; 
uint32_t sensorArrayIndex = 0, tmpArrayIndex = 0;
uint8_t collectFinishedCond = 0;
uint32_t lastMeaureTime = 0;
//--------------------------------

void setup(){
  // UART設置
  Serial.begin(115200);

  // mpu6050設置
  mpu6050.init();
  while(!mpu6050.isReady()); //wait for mpu6050 get ready

  // GPIO設置
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BTN_PIN,INPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
}

void loop(){
  static bool collect = false;
  static bool collectStage = 0;
  static bool showStageInfo = false;
  
  //100ms取一次, 共取10次, 也就是一秒
  if(millis() - lastMeaureTime > 100 && !collect){
    //mpu6050資料更新  
    mpu6050.update();
    switch(collectStage){
      case 0: 
        if(mpu6050.data.gyrX > 150 || mpu6050.data.gyrX < -150 ||
          mpu6050.data.gyrY > 150 || mpu6050.data.gyrY < -150 ||
          mpu6050.data.gyrZ > 150 || mpu6050.data.gyrZ < -150 ||
          mpu6050.data.accX > 0.25 || mpu6050.data.accX < -0.25 ||
          mpu6050.data.accY > -0.75  || mpu6050.data.accY < -1.25 ||
          mpu6050.data.accZ > 0.25 || mpu6050.data.accZ < -0.25 )
        {
          collect = true;
        }
        break;
      case 1:
        if(mpu6050.data.gyrX > 150 || mpu6050.data.gyrX < -150 ||
          mpu6050.data.gyrY > 150 || mpu6050.data.gyrY < -150 ||
          mpu6050.data.gyrZ > 150 || mpu6050.data.gyrZ < -150  )
        {
          collect = true;
        }
        break;
    }
    
    lastMeaureTime = millis();
  }

  

  // 當按鈕按下時開始收集資料   
  if(collect) {
    // 收集資料時, 內建指示燈會亮
    digitalWrite(LED_BUILTIN, LED_ON);
    if(millis() - lastMeaureTime > 100){
      //mpu6050資料更新  
      mpu6050.update();

      if(collectFinishedCond == PERIOD){
        Serial.println("此筆資料收集已完成, 可以進行下一筆資料收集");
        showStageInfo = true;
        collect = false;
      }else{
        sensor_data[sensorArrayIndex] = mpu6050.data.accX; sensorArrayIndex++;
        sensor_data[sensorArrayIndex] = mpu6050.data.accY; sensorArrayIndex++;
        sensor_data[sensorArrayIndex] = mpu6050.data.accZ; sensorArrayIndex++;
        sensor_data[sensorArrayIndex] = mpu6050.data.gyrX; sensorArrayIndex++;
        sensor_data[sensorArrayIndex] = mpu6050.data.gyrY; sensorArrayIndex++;
        sensor_data[sensorArrayIndex] = mpu6050.data.gyrZ; sensorArrayIndex++;
        collectFinishedCond++;
      }
      lastMeaureTime = millis();
    }
  }else{
    // 按鈕放開, 則代表資料要重新收集
    digitalWrite(LED_BUILTIN, LED_OFF);
    collectFinishedCond = 0;

     // 假設中途手放開btn則不足以形成一筆Feature data
    if(sensorArrayIndex % FEATURE_DIM != 0) sensorArrayIndex = tmpArrayIndex;
    else                                    tmpArrayIndex = sensorArrayIndex;

    //每一個階段都會提示該階段收集完成的訊息
    if(showStageInfo){
      if(sensorArrayIndex == FEATURE_LEN / 2){
        Serial.println("Others Feature Sample Done");
        Serial.println("");
        showStageInfo = false;
        collectStage = 1;
      }else if(sensorArrayIndex == FEATURE_LEN){
        Serial.println("Work Feature Sample Done");
        Serial.println("");
        showStageInfo = false;
        
        sensorArrayIndex = 0;
        uint32_t showIndex = 0;

        // 匯出others.txt
        Serial.println("others.txt");
        for(uint32_t x = 0; x < ROUND; x++){
          for(uint32_t y = 0; y < FEATURE_DIM; y++){
            Serial.print(sensor_data[x * FEATURE_DIM + y]);
            if(y < FEATURE_DIM - 1 ) Serial.print(",");
            showIndex++;
          }
          Serial.println();
        }
        Serial.println();
        
        // 匯出falling.txt
        Serial.println("fall.txt");
        for(uint32_t x = 0; x < ROUND; x++){
          for(uint32_t y = 0; y < FEATURE_DIM; y++){
            Serial.print(sensor_data[showIndex + x * FEATURE_DIM + y]);
            if(y < FEATURE_DIM - 1 ) Serial.print(",");
          }
          Serial.println();
        }
        Serial.println();

        while(1);
      }
    }
  }
}  
