/*
  控制伺服馬達
*/
#include <ESP32Servo.h>

Servo servo;

void setup(){  
  // 設定伺服馬達的接腳
  servo.attach(33, 500, 2400); 
}

void loop(){
  for(int angle = 0; angle < 180; angle++){
    servo.write(angle);
    delay(15); 
  }
  for(int angle = 180; angle > 0; angle--){
    servo.write(angle);
    delay(15); 
  }
}