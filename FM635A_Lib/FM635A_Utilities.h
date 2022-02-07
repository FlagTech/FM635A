#ifndef _FM635A_UTILITIES_H
#define _FM635A_UTILITIES_H

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

namespace FM635A_Utilities
{
  enum{
    LED_ON,
    LED_OFF
  };

  class SwitchWithDebounce {
    public:
      SwitchWithDebounce(uint8_t pinNum, uint8_t type){
        _stableTime = 20;
        _pinNum = pinNum;
        pinMode(_pinNum, type);
        _lastState = digitalRead(_pinNum);;
      }

      int8_t read(){
        int reading = digitalRead(_pinNum);

        // 開關狀態改變, 可能是: 1. 按下/放開開關 2. 開關彈跳造成的
        if (reading != _lastState) {
          // 重置計算穩定時間的計時器
          _lastTime = millis();
        }

        _lastState = reading; 

        if ((millis() - _lastTime) > _stableTime) return reading; //開關已穩定而不彈跳, 回傳讀到的按鈕輸入值
        else                                      return -1;      //開關仍在彈跳而不穩定         
      }

    private:
      uint8_t _pinNum = 0;
      uint32_t _lastTime = 0;
      uint8_t _lastState = 0;
      uint8_t _stableTime = 0;
  };
  
  //統計物件用的選項
  enum{
    DATA_TYPE_F32,
  };

  //統計物件用的選項
  enum{
    STATISTICS_POPULATION,
    STATISTICS_SAMPLE,
  };

  class Statistics{

    public :
      struct{
        uint32_t dataLen;
        uint8_t dataType;
        void *paraBuf; 
      }mean_data_config;
      
      struct{
        uint32_t dataLen;
        uint8_t dataType;
        uint8_t stdType; 
        void *paraBuf; 
      }std_data_config;

      Statistics(){}

      //平均值算法
      float mean(){
        float sum = 0;
        if(mean_data_config.dataType == DATA_TYPE_F32){ 
          for(int i = 0; i<mean_data_config.dataLen; i++){
            sum+=((float*) mean_data_config.paraBuf)[i];
          }
          sum /= mean_data_config.dataLen; 
        }else{
          //TODO
        }
        return sum;
      }

      //標準差算法
      float std(){
        float std=0;
        if(std_data_config.stdType == STATISTICS_POPULATION){
          //母體標準差
          mean_data_config.dataLen = std_data_config.dataLen;
          mean_data_config.dataType = std_data_config.dataType;
          mean_data_config.paraBuf = std_data_config.paraBuf;
          float Mean = mean();
          float sum = 0;
          if(mean_data_config.dataType == DATA_TYPE_F32){ 
            for(int i = 0; i<std_data_config.dataLen; i++){
              sum += sq( ((float*)std_data_config.paraBuf)[i] - Mean );
            }
          }else{
            //TODO
          }
          sum /= std_data_config.dataLen;
          return sqrt(sum);
        }else{
          //樣本標準差
          mean_data_config.dataLen = std_data_config.dataLen;
          mean_data_config.dataType = std_data_config.dataType;
          float Mean = mean();
          float sum = 0;
          if(mean_data_config.dataType == DATA_TYPE_F32){ 
            for(int i = 0; i<std_data_config.dataLen; i++){
              sum += sq( ((float*)std_data_config.paraBuf)[i] - Mean );
            }
          }else{
            //TODO
          }
          sum /= std_data_config.dataLen - 1;
          return sqrt(sum);
        }
      }
  };

  class Queue{
    public:
      uint16_t buf[40];
      uint32_t in;
      uint32_t out;

      Queue(){
        memset(buf,0,sizeof(buf));
        in = 0;
        out = 0;
      }

      uint8_t push(uint16_t data){
        if(isFull()){
          return false;
        }else{
          buf[in] = data;
          in++;
          in %= (sizeof(buf)/sizeof(buf[0]));
          return true;
        }
      }

      void push_force(uint16_t data){
        if(isFull()){
          uint16_t tmp;
          pop(&tmp);
        }
        buf[in] = data;
        in++;
        in %= (sizeof(buf)/sizeof(buf[0]));
      }

      uint8_t pop(uint16_t *data){
        if(isEmpty()){
          return false;
        }else{
          *data = buf[out]; 
          out++;
          out %= (sizeof(buf)/sizeof(buf[0]));
          return true;
        }
      }

      uint8_t isEmpty(){
        if(in == out) return true;
        else          return false;
      }

      uint8_t isFull(){
        if((in+1) % (sizeof(buf)/sizeof(buf[0])) == out) return true;
        else                                             return false;
      }

      uint16_t avg(){
        uint32_t sum = 0;
        for(int i = 0; i<sizeof(buf)/sizeof(buf[0]); i++){
          sum += buf[i];
        }
        sum /= sizeof(buf)/sizeof(buf[0]);
        return (uint16_t) sum;
      }
  };

  typedef struct Task_t{
    uint8_t active;
    uint32_t period;
    void (*callback)(void); 
    uint32_t timer;
    struct Task_t *next;
  }Task;

  class TaskManager{
    public:
      Task *head;
      Task *current;
      uint8_t taskTotal;

      TaskManager(){
        head = NULL;
        current = NULL;
        taskTotal = 0;
      }

      void add(Task *task){
        task->next = NULL; //clear next of task
        task->timer = 0;   //reset task timer
        if(taskTotal == 0){ 
          //add first task
          head = task;
          current = head;
        }else{
          current->next = task;
          current = task;
        }
        taskTotal++;
      }

      void execute(){
        if(current->active){
          if(millis()- current->timer >= current->period){
            (*current->callback)();
            current->timer = millis();
          }
        }
        if(current->next == NULL) current = head;
        else                      current = current->next;
      }
  };

  enum{
    MPU6050_ACC_RANGE_2G,
    MPU6050_ACC_RANGE_4G, 
    MPU6050_ACC_RANGE_8G,
    MPU6050_ACC_RANGE_16G,
  };

  enum{
    MPU6050_GYR_RANGE_250_PER_SEC,
    MPU6050_GYR_RANGE_500_PER_SEC, 
    MPU6050_GYR_RANGE_1000_PER_SEC,
    MPU6050_GYR_RANGE_2000_PER_SEC
  };

  class MPU6050{
    public:
      uint16_t i2c_addr;

      struct{
        volatile int16_t accX;
        volatile int16_t accY;
        volatile int16_t accZ;
        volatile int16_t temperature;
        volatile int16_t gyrX;
        volatile int16_t gyrY;
        volatile int16_t gyrZ;
      }rawData;
      
      struct{
        volatile float accX;
        volatile float accY;
        volatile float accZ;
        volatile float temperature;
        volatile float gyrX;
        volatile float gyrY;
        volatile float gyrZ;
      }data;

      MPU6050(){
        i2c_addr = 0x68;
        _accRange = MPU6050_ACC_RANGE_2G;
        _gyrRange = MPU6050_GYR_RANGE_250_PER_SEC;

        memset(&rawData,0,sizeof(rawData));
        memset(&data,0,sizeof(data));
      }

      void init(int sda_pin = 26, int scl_pin = 25, uint16_t i2c_addr = 0x68){
        //取決於MPU6050 A0 pin:
        //Lo => 0x68
        //Hi => 0x69
        this->i2c_addr = i2c_addr;
        
        //會使用到Wire庫
        Wire.begin(21,22); 
  
        //detect MPU6050
        Wire.beginTransmission(i2c_addr);
        if (Wire.endTransmission(true) == 0) Serial.println("MPU6050 detect suceess");
        else                                 Serial.println("MPU6050 detect fail");
  
        //check & show slave addr
        Wire.beginTransmission(i2c_addr);
        Wire.write(0x75);
        Wire.endTransmission(false);                   //false表示repeat start
        Wire.requestFrom(i2c_addr, (uint8_t)1, true);  //結束傳輸，true表示釋放總線
        Serial.println(Wire.read());

        //初始化只要對0x6B寫入0即可
        Wire.beginTransmission(i2c_addr); //開啟MPU6050的傳輸
        Wire.write(0x6B);                 //指定寄存器地址
        Wire.write(0);                    //寫入一個字節的數據
        Wire.endTransmission(true);       //結束傳輸，true表示釋放總線

        setAccRange(MPU6050_ACC_RANGE_2G);
        setGyrRange(MPU6050_GYR_RANGE_250_PER_SEC);
      }

      uint8_t isReady(){
        update();
        if(rawData.accX == 0 && rawData.accY == 0 && rawData.accZ == 0) return false;
        else                                                            return true;
      }

      void update(){
        // 我們感興趣的數據位於0x3B到0x48這14個字節的寄存器中。
        // 這些數據會被動態更新，更新頻率最高可達1000HZ。
        // 下面列出相關寄存器的地址，數據的名稱。註意，每個數據都是2個字節。
        // 0x3B，加速度計的X軸分量ACC_X
        // 0x3D，加速度計的Y軸分量ACC_Y
        // 0x3F，加速度計的Z軸分量ACC_Z
        // 0x41，當前溫度TEMP
        // 0x43，繞X軸旋轉的角速度GYR_X
        // 0x45，繞Y軸旋轉的角速度GYR_Y
        // 0x47，繞Z軸旋轉的角速度GYR_Z
        Wire.beginTransmission(i2c_addr);
        Wire.write(0x3B); 
        Wire.endTransmission(false);                    //false表示repeat start
        Wire.requestFrom(i2c_addr, (uint8_t)14, true);  //結束傳輸，true表示釋放總線
        
        rawData.accX = Wire.read() << 8 | Wire.read();
        rawData.accY = Wire.read() << 8 | Wire.read();
        rawData.accZ = Wire.read() << 8 | Wire.read();
        rawData.temperature = Wire.read() << 8 | Wire.read();
        rawData.gyrX = Wire.read() << 8 | Wire.read();
        rawData.gyrY = Wire.read() << 8 | Wire.read();
        rawData.gyrZ = Wire.read() << 8 | Wire.read();

        data.temperature = (rawData.temperature / 340.0) + 36.53; //ref to adafruit's mpu6050 lib

        switch(_accRange){
          case MPU6050_ACC_RANGE_2G:
            data.accX = (float)rawData.accX / 32768.0 * 2.0;
            data.accY = (float)rawData.accY / 32768.0 * 2.0;
            data.accZ = (float)rawData.accZ / 32768.0 * 2.0;
            break;
          case MPU6050_ACC_RANGE_4G:
            data.accX = (float)rawData.accX / 32768.0 * 4.0;
            data.accY = (float)rawData.accY / 32768.0 * 4.0;
            data.accZ = (float)rawData.accZ / 32768.0 * 4.0;
            break;
          case MPU6050_ACC_RANGE_8G:
            data.accX = (float)rawData.accX / 32768.0 * 8.0;
            data.accY = (float)rawData.accY / 32768.0 * 8.0;
            data.accZ = (float)rawData.accZ / 32768.0 * 8.0;
            break;
          case MPU6050_ACC_RANGE_16G:
            data.accX = (float)rawData.accX / 32768.0 * 16.0;
            data.accY = (float)rawData.accY / 32768.0 * 16.0;
            data.accZ = (float)rawData.accZ / 32768.0 * 16.0;
            break;
        }

        switch(_gyrRange){
          case MPU6050_GYR_RANGE_250_PER_SEC:
            data.gyrX = (float)rawData.gyrX / 32768.0 * 250.0;
            data.gyrY = (float)rawData.gyrY / 32768.0 * 250.0;
            data.gyrZ = (float)rawData.gyrZ / 32768.0 * 250.0;
            break;
          case MPU6050_GYR_RANGE_500_PER_SEC:
            data.gyrX = (float)rawData.gyrX / 32768.0 * 500.0;
            data.gyrY = (float)rawData.gyrY / 32768.0 * 500.0;
            data.gyrZ = (float)rawData.gyrZ / 32768.0 * 500.0;
            break;
          case MPU6050_GYR_RANGE_1000_PER_SEC:
            data.gyrX = (float)rawData.gyrX / 32768.0 * 1000.0;
            data.gyrY = (float)rawData.gyrY / 32768.0 * 1000.0;
            data.gyrZ = (float)rawData.gyrZ / 32768.0 * 1000.0;
            break;
          case MPU6050_GYR_RANGE_2000_PER_SEC:
            data.gyrX = (float)rawData.gyrX / 32768.0 * 2000.0;
            data.gyrY = (float)rawData.gyrY / 32768.0 * 2000.0;
            data.gyrZ = (float)rawData.gyrZ / 32768.0 * 2000.0;
            break;
        }
      }
      
      void setAccRange(uint8_t range){
        Wire.beginTransmission(i2c_addr);              
        Wire.write(0x1C);                              //加速度倍率寄存器的地址
        Wire.endTransmission(false);                   //repeat start

        Wire.requestFrom(i2c_addr, (uint8_t)1, true); //先讀出原配置, 目的是除了acc range配置要修改外, 其餘都要保持原狀
        unsigned char acc_conf = Wire.read();
        acc_conf = ((acc_conf & 0xE7) | (range << 3)); //clear acc range的配置bit, 然後重新set range
        
        Wire.beginTransmission(i2c_addr);            
        Wire.write(0x1C);    
        Wire.write(acc_conf);
        Wire.endTransmission(true);                    
      }

      void setGyrRange(uint8_t range){
        Wire.beginTransmission(i2c_addr);              //開啟MPU-6050的傳輸
        Wire.write(0x1B);                              //角速度倍率寄存器的地址
        Wire.endTransmission(false);                   //repeat start

        Wire.requestFrom(i2c_addr, (uint8_t)1, true);  //先讀出原配置, 目的是除了gyr range配置要修改外, 其餘都要保持原狀
        unsigned char gyr_conf = Wire.read();
        gyr_conf = ((gyr_conf & 0xE7) | (range << 3)); //clear gyr range的配置bit, 然後重新set range

        Wire.beginTransmission(i2c_addr);              //開啟MPU-6050的傳輸
        Wire.write(0x1B);                              //角速度倍率寄存器的地址
        Wire.write(gyr_conf);
        Wire.endTransmission(true);                    //結束傳輸，true表示釋放總線
      }

    private:
      uint8_t _accRange;
      uint8_t _gyrRange;
      
      void _readData(uint8_t reg, uint8_t *data, uint8_t dataLen){
        Wire.beginTransmission(i2c_addr);          //開啟MPU6050的傳輸, 使用寫地址, 因為下一行要告知欲讀的Register addr
        Wire.write(reg);                           //指定寄存器地址
        Wire.endTransmission(false);               

        Wire.requestFrom(i2c_addr, dataLen, true); //將輸據讀出到緩存(repeat start機制, 目的是讓master不釋放bus, 直接下讀地址進行Register讀取)
        
        for(int i = 0; i < dataLen; i++){
          data[i] = Wire.read();                   //依序讀取每一個byte, 超過當前寄存器長度的將讀取到後面地址的寄存器內容
        }
      }

      void _writeData(uint8_t reg, uint8_t *data, uint8_t dataLen){
        Wire.beginTransmission(i2c_addr); //開啟MPU6050的傳輸
        Wire.write(reg);                  //指定寄存器地址
        for(int i = 0; i < dataLen; i++){
          Wire.write(data[i]);            //依序寫入每一個byte, 超過當前寄存器長度的將寫入到後面地址的寄存器中
        }
        Wire.endTransmission(true);       //結束傳輸，true表示釋放總線
      }
  };
}

#endif
