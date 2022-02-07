#include "FM635A_Utilities.h"

#define DEG_TO_RAD 0.017453293F

FM635A_Utilities::MPU6050 mpu6050;
FM635A_Utilities::TaskManager taskManager;

//更新mpu6050的資訊
FM635A_Utilities::Task readMPU6050 = {
  .active = true, 
  .period = 100, 
  .callback = [](){
  mpu6050.update();
}};

//顯示mpu6050資訊
FM635A_Utilities::Task showMPU6050 = {
  .active = false, 
  .period = 500, 
  .callback = [](){
  Serial.print(F("ACC_X: ")); Serial.println(mpu6050.data.accX);
  Serial.print(F("ACC_Y: ")); Serial.println(mpu6050.data.accY);
  Serial.print(F("ACC_Z: ")); Serial.println(mpu6050.data.accZ);
  Serial.print(F("GYR_X: ")); Serial.println(mpu6050.data.gyrX * DEG_TO_RAD);
  Serial.print(F("GYR_Y: ")); Serial.println(mpu6050.data.gyrY * DEG_TO_RAD);
  Serial.print(F("GYR_Z: ")); Serial.println(mpu6050.data.gyrZ * DEG_TO_RAD);
  Serial.print(F("Temperature: ")); Serial.println(mpu6050.data.temperature);
  Serial.println(F(""));
}};

void initMCU(){
  Serial.begin(115200);

  mpu6050.init();
  while(!mpu6050.isReady()); //等待mpu6050
}

void setup(){
  //初始化MCU外設
  initMCU();

  //註冊task到taskManager
  taskManager.add(&readMPU6050);
  taskManager.add(&showMPU6050);
}

void loop(){
  taskManager.execute();
}