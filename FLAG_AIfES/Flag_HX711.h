#ifndef FLAG_HX711_H
#define FLAG_HX711_H

#include <Arduino.h>
#include <Flag_Queue.h>

#define pulse(pin) { digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

class Flag_HX711
{
    private:
        uint8_t SCK;
        uint8_t DOUT;
        uint8_t AMP;
        int32_t OFFSET;
        float COEFFICIENT;
        Flag_Queue _que;

    public:
        Flag_HX711(uint8_t sck, uint8_t dout, uint8_t amp = 128, float co =  0.0011545){
            SCK = sck;
            DOUT = dout;
            setAmp(amp);
            COEFFICIENT = co;
            pinMode(SCK, OUTPUT);
            pinMode(DOUT, INPUT);
            digitalWrite(SCK, LOW);
            
            //首次先把que塞滿
            for(uint32_t  i = 0; i < _que.getBufSize() * 2; i++){
                readRawData();
            }
        }

        void setAmp(uint8_t amp){
            switch (amp) {
                case 32:  AMP = 2; break;
                case 64:  AMP = 3; break;
                case 128: AMP = 1; break;
            }
        }

        bool isReady(){
            return digitalRead(DOUT) == LOW;
        }

        int32_t readRawData(){
            int32_t val = 0;

            while (!isReady());

            for (int i = 0; i < 24; i++) {
                pulse(SCK);
                val <<= 1;
                if (digitalRead(DOUT) == HIGH) val++;
            }

            for (int i = 0; i < AMP; i++) {
                pulse(SCK);
            }
            
            val = val & (1L << 23) ? val | ((-1L) << 24) : val;
            _que.push_force(val);
            return val;
        }

        void tare(){
            // 取移動平均 (平均_que.buffer size筆)
            readRawData();
            setOffset(_que.avg());
        }

        void setCoefficient(float co = 0.0011545){
            COEFFICIENT = co;
        }

        void setOffset(int32_t offset = 0){
            OFFSET = offset;
        }

        // 取得weight平均值
        float getWeight() {
            // 取移動平均 (平均_que.buffer size筆)
            readRawData();
            float weight = (_que.avg() - OFFSET) * COEFFICIENT; 
            return weight; 
        }

        // 取得offset平均值
        int32_t getOffset(){
            return OFFSET;
        }
 
        // 取得係數
        float getCoefficient(){
            return COEFFICIENT;
        }
};

#endif
