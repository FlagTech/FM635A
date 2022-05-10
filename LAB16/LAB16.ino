/*
  顯示六軸感測資訊
*/
#include <Flag_MPU6050.h>

Flag_MPU6050 mpu6050;

void setup(){
  // 序列埠設置
  Serial.begin(115200);

  // MPU6050 初始化
  mpu6050.init();
  while(!mpu6050.isReady()); 
}

void loop(){
  // 更新 MPU6050 的資訊
  mpu6050.update();

  // 顯示 MPU6050 資訊
  Serial.print("ACC_X: "); 
  Serial.println(mpu6050.data.accX);
  Serial.print("ACC_Y: "); 
  Serial.println(mpu6050.data.accY);
  Serial.print("ACC_Z: "); 
  Serial.println(mpu6050.data.accZ);
  Serial.print("GYR_X: "); 
  Serial.println(mpu6050.data.gyrX);
  Serial.print("GYR_Y: "); 
  Serial.println(mpu6050.data.gyrY);
  Serial.print("GYR_Z: "); 
  Serial.println(mpu6050.data.gyrZ);
  Serial.print("Temperature: "); 
  Serial.println(mpu6050.data.temperature);
  Serial.println();

  delay(1000);
}