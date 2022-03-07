#ifndef _FLAG_SWITCH_H
#define _FLAG_SWITCH_H

#include <Arduino.h>

class Flag_Switch {
  public:
    Flag_Switch(uint8_t pinNum, uint8_t type){
      _stableTime = 20;
      _pinNum = pinNum;
      pinMode(_pinNum, type);
      _lastStableState = _triggerEdge = digitalRead(_pinNum);
      _detectFirst = 1;
    }

    int8_t read(){
      int reading = digitalRead(_pinNum);

      // 開關狀態改變, 可能是: 1. 按下/放開開關 2. 開關彈跳造成的
      if (reading != _lastStableState && _detectFirst == 1) {
        // 重置計算穩定時間的計時器
        _detectFirst = 0;
        _lastTime = millis();
        _triggerEdge = reading; 
        return _triggerEdge; 
      }else{
        if ((millis() - _lastTime) > _stableTime){
          _detectFirst = 1;
          _lastStableState = reading; //更新穩定值
          return _lastStableState;    //開關已穩定而不彈跳, 回傳讀到的按鈕輸入值
        } else {
          return _triggerEdge;
        }
      }
    }

  private:
    uint8_t _pinNum = 0;
    uint32_t _lastTime = 0;
    uint8_t _triggerEdge = 0;
    uint8_t _lastStableState = 0;
    uint8_t _stableTime = 0;
    uint8_t _detectFirst = 1;
};

#endif