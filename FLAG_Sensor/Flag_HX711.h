#ifndef FLAG_HX711_H
#define FLAG_HX711_H

#include <Arduino.h>
#include <Flag_Queue.h>

#define FLAG_HX711_PULSE(pin) { digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

// -----------------------------------------
// 下述gain / offset 是底下數學函數的參數
// adc = f(weight) = gain * weight + offset
// gain與offset使用2點校正法獲得
// -----------------------------------------
#define FLAG_HX711_DEFAULT_GAIN     (-2117.48)
#define FLAG_HX711_DEFAULT_OFFSET   (-195250)

// 這是為了方便教學神經網路而調整的, 其實電子秤本身的<weight-adc>就足夠線性
#define FLAG_HX711_ADJ_GAIN   0.97

// 若想直接使用滿準的線性近似, 使用底下這兩行
// #define FLAG_HX711_ADJ_GAIN   1

#define FLAG_HX711_GAIN    (FLAG_HX711_DEFAULT_GAIN * FLAG_HX711_ADJ_GAIN)
#define FLAG_HX711_OFFSET  (FLAG_HX711_DEFAULT_OFFSET)

class Flag_HX711
{
    private:
        uint8_t SCK;
        uint8_t DOUT;
        uint8_t AMP;
        Flag_Queue _que;
        int32_t offset;
        float gain;
        float refWeight;
    public:
        Flag_HX711(uint8_t sck, uint8_t dout, uint8_t amp = 128){
            SCK = sck;
            DOUT = dout;
            setAmp(amp);
            offset  = FLAG_HX711_OFFSET;
            gain = FLAG_HX711_GAIN;
            refWeight = 0;
        }

        void begin(){    
            pinMode(SCK, OUTPUT);
            pinMode(DOUT, INPUT);
            digitalWrite(SCK, LOW);
            
            //首次先把que塞滿3次, 只是穩定用而已
            for(uint32_t  i = 0; i < _que.getBufSize() * 3; i++){
                readRawData();
            }
        }

        bool isReady(){
            return digitalRead(DOUT) == LOW;
        }

        // 讀取一筆raw data
        int32_t readRawData(){
            int32_t val = 0;

            while (!isReady());

            for (int i = 0; i < 24; i++) {
                FLAG_HX711_PULSE(SCK);
                val <<= 1;
                if (digitalRead(DOUT) == HIGH) val++;
            }

            for (int i = 0; i < AMP; i++) {
                FLAG_HX711_PULSE(SCK);
            }
            
            val = val & (1L << 23) ? val | ((-1L) << 24) : val;
            _que.push_force(val);
            return val;
        }

        void setOffset(int32_t offset){
            this->offset = offset;
        }
        
        void setGain(float gain){
            this->gain = gain;
        }
   
        // 取得offset平均值
        int32_t getOffset(){
            return offset;
        }
 
        // 取得係數
        float getGain(){
            return gain;
        }
        
        int32_t getAdc(){
           // 平均_que.buffer size筆
            for(uint32_t  i = 0; i < _que.getBufSize(); i++){
                readRawData();
            }
            return _que.avg(); 
        }

        // 取得weight平均值
        float getWeight() {
            // 平均_que.buffer size筆
            for(uint32_t  i = 0; i < _que.getBufSize(); i++){
                readRawData();
            }
            float weight = (_que.avg() - offset) / gain; 
            weight -= refWeight;
            return weight; 
        }
        
        float getWeightAsync() {
            // 取移動平均
            readRawData();
            float weight = (_que.avg() - offset) / gain; 
            weight -= refWeight;
            return weight; 
        }

        void setAmp(uint8_t amp){
            switch (amp) {
                case 32:  AMP = 2; break;
                case 64:  AMP = 3; break;
                case 128: AMP = 1; break;
            }
        }

        void tare(){
            refWeight = getWeight();
        }
};

#endif
