/*
  控制伺服馬達
*/
#include <ESP32_Servo.h>

#define SERVO_PIN 33

Servo servo;

void setup(){  
  servo.attach(SERVO_PIN, 500, 2400); // 設定伺服馬達的接腳
}

void loop(){
  for(int angle = 0; angle < 180; angle++){
    servo.write(angle);
    delay(15); // 延遲一段時間，讓伺服馬達轉到定位。 
  }
  for(int angle = 180; angle > 0; angle--){
    servo.write(angle);
    delay(15); // 延遲一段時間，讓伺服馬達轉到定位。 
  }
}