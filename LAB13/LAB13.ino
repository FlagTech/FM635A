#include <Flag_MPU6050.h>

Flag_MPU6050 mpu6050;

void setup(){
  // UART init
  Serial.begin(115200);

  // mpu6050 init
  mpu6050.init();
  while(!mpu6050.isReady()); 
}

void loop(){
  // 更新mpu6050的資訊
  mpu6050.update();

  // 顯示mpu6050資訊
  Serial.print(F("ACC_X: ")); Serial.println(mpu6050.data.accX);
  Serial.print(F("ACC_Y: ")); Serial.println(mpu6050.data.accY);
  Serial.print(F("ACC_Z: ")); Serial.println(mpu6050.data.accZ);
  Serial.print(F("GYR_X: ")); Serial.println(mpu6050.data.gyrX);
  Serial.print(F("GYR_Y: ")); Serial.println(mpu6050.data.gyrY);
  Serial.print(F("GYR_Z: ")); Serial.println(mpu6050.data.gyrZ);
  Serial.print(F("Temperature: ")); Serial.println(mpu6050.data.temperature);
  Serial.println(F(""));

  delay(1000);
}