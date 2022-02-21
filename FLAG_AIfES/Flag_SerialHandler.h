#ifndef _FLAG_SERIALHANDLER_H
#define _FLAG_SERIALHANDLER_H

class Flag_SerialHandler{
  public:
    //debug info選項
    enum{
      INFO_VERBOSE,
      INFO_SIMPLE,
      INFO_BASE,
    };

    float *buf;
    
    //Funtion Name: SerialHandler
    //Purpose: 建構子
    //Parameter: None
    //Return: None
    SerialHandler(){
      _debugInfoType = INFO_BASE;
    }

    //Funtion Name: debugInfoTypeConfig
    //Purpose: 設定debug訊息種類
    //Parameter:
    //  infoType: 可以選擇INFO_VERBOSE, INFO_SIMPLE, INFO_BASE
    //Return: None
    void debugInfoTypeConfig(uint8_t infoType = INFO_BASE){
      _debugInfoType = infoType;
    }

    //Funtion Name: handle
    //Purpose: 儲存一道使用sep分隔資料, 並使用\n結尾的輸入字串訊息到成員變數的buf中
    //Parameter:
    //  sep: 可以自訂分隔資料的字元
    //  end: 可以自訂以結束字元
    //Return: 資料筆數
    uint32_t handle(char sep = ' ', char end = '\n'){
      while(1){
        if(Serial.available() > 0 ){
          String str = Serial.readStringUntil(end);
          str += end; //因為 Serial.readStringUntil()不會讀到end
        
          String strTemp = "";
          uint32_t strIndex = 0;
          uint32_t inputNum=0;
          
          //先算資料筆數
          while(1){
            strTemp += str[strIndex];
            strIndex++;
            if(str[strIndex] == sep){
              inputNum++;
              strTemp = "";
              strIndex++;
            }else if(str[strIndex] == end){
              inputNum++;
              strTemp = "";
              strIndex++;
              break;
            }
          }

          //動態記憶體配置
          buf = (float *) malloc(sizeof(float) * inputNum);
          
          //分配完後才可以寫到buf
          strTemp = "";
          strIndex = 0;
          inputNum=0;
          while(1){
            strTemp += str[strIndex];
            strIndex++;
            if(str[strIndex] == sep){
              buf[inputNum] = strTemp.toFloat();
              inputNum++;
              strTemp = "";
              strIndex++;
            }else if(str[strIndex] == end){
              buf[inputNum] = strTemp.toFloat();
              inputNum++;
              strTemp = "";
              strIndex++;
              return inputNum;
            }
          }
        }
      }
    }

    //Funtion Name: freeMemory
    //Purpose: 釋放buf所指的動態分配的記憶體
    //Parameter: None
    //Return: None
    void freeMemory(){
      free(buf);
    }

  private:
    uint8_t _debugInfoType;
};

#endif