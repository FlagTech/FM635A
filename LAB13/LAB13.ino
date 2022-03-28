/*
  色彩與手勢偵測感測器
*/
#include <Wire.h>
#include <SparkFun_APDS9960.h>

#define LED_ON  0
#define LED_OFF 1

//------------全域變數------------
// 感測器的物件
SparkFun_APDS9960 apds = SparkFun_APDS9960();
//--------------------------------

void setup() {
  // UART設置
  Serial.begin(115200);

  // 初始化APDS9960
  if(apds.init()) Serial.println(F("APDS-9960 初始化完成"));
  else            Serial.println(F("APDS-9960 初始化錯誤"));
  
  // 啟用APDS-9960光傳感器
  if(apds.enableLightSensor(false)) Serial.println(F("光傳感器正在運行"));
  else                              Serial.println(F("光傳感器初始化錯誤"));
  
  // 調整接近傳感器增益
  if(!apds.setProximityGain(PGAIN_2X)) Serial.println(F("設置 PGAIN 時出現問題"));
  
  // 啟用APDS-9960接近傳感器
  if(apds.enableProximitySensor(false)) Serial.println(F("接近傳感器正在運行"));
  else                                  Serial.println(F("傳感器初始化錯誤"));
  
  // GPIO設置
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
}

void loop(){
  uint8_t proximity_data = 0;
  uint16_t red_light = 0, green_light = 0, blue_light = 0, ambient_light = 0;

  // 接近測試
  if(!apds.readProximity(proximity_data)){
    Serial.println("此次讀取接近值錯誤");
  }

  // RGB偵測
  if (proximity_data == 255) {
    // 偵測時指示燈會亮
    digitalWrite(LED_BUILTIN, LED_ON);

    if(!apds.readAmbientLight(ambient_light)  ||
       !apds.readRedLight(red_light)          ||
       !apds.readGreenLight(green_light)      ||
       !apds.readBlueLight(blue_light)){
      Serial.println("Error reading light values");
    }else{
      Serial.print("Ambient: ");
      Serial.print(ambient_light);
      Serial.print(" Red: ");
      Serial.print(red_light);
      Serial.print(" Green: ");
      Serial.print(green_light);
      Serial.print(" Blue: ");
      Serial.println(blue_light);
    }
  } else {
    // 未偵測時指示燈會不亮
    digitalWrite(LED_BUILTIN, LED_OFF);
  }

  delay(1000);
}
