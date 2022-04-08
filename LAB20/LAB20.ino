/*
  控制伺服馬達
*/
#include <ESP32_Servo.h>

Servo servo;

void setup(){  
  Serial.begin(115200);
  // 設定伺服馬達的接腳
  servo.attach(33, 500, 2400); 
}

void loop(){
  Serial.println("test");
  for(int angle = 0; angle < 180; angle++){
    servo.write(angle);
    delay(15); 
  }
  for(int angle = 180; angle > 0; angle--){
    servo.write(angle);
    delay(15); 
  }
}