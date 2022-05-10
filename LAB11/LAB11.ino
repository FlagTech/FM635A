/*
  色彩與接近偵測感測器
*/
#include <Wire.h>
#include <SparkFun_APDS9960.h>

//------------全域變數------------
// 感測器的物件
SparkFun_APDS9960 apds = SparkFun_APDS9960();
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

  // 接近測試
  if(!apds.readProximity(proximity_data)){
    Serial.println("此次讀取接近值錯誤");
  }
  Serial.print("接近值: ");
  Serial.println(proximity_data);

  // RGB 偵測
  if(proximity_data == 255){
    // 偵測時指示燈會亮
    digitalWrite(LED_BUILTIN, LOW);

    if(!apds.readAmbientLight(ambient_light)  ||
       !apds.readRedLight(red_light)          ||
       !apds.readGreenLight(green_light)      ||
       !apds.readBlueLight(blue_light)){
      Serial.println("讀值錯誤");
    }else{
      Serial.print("環境光: ");
      Serial.print(ambient_light);
      Serial.print(" 紅光: ");
      Serial.print(red_light);
      Serial.print(" 綠光: ");
      Serial.print(green_light);
      Serial.print(" 藍光: ");
      Serial.println(blue_light);
    }
  }else{
    // 未偵測時指示燈不會亮
    digitalWrite(LED_BUILTIN, HIGH);
  }
  delay(1000);
}
