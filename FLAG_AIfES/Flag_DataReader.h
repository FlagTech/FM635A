#ifndef _FLAG_DATAREADER_H
#define _FLAG_DATAREADER_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <Flag_Statistics.h>

typedef struct {
  float *feature;   //data
  float *label;     //label
  uint32_t dataLen; //資料筆數
  uint32_t featureDim; //特徵輸入維度
  uint32_t labelDim;   //label維度
  uint32_t featureDataArryLen; //data的陣列長度(以1維陣列來看)
  uint32_t labelDataArryLen;   //label的陣列長度(以1維陣列來看)
  float featureMean; //特徵資料平均值
  float featureSd;   //特徵資料標準差
  float labelMaxAbs;    //標籤資料最大值(僅用於迴歸分析)
}Flag_DataBuffer;

class Flag_DataReader {

  public :
    //debug info選項
    enum{
      INFO_VERBOSE,
      INFO_SIMPLE,
      INFO_BASE,
    };

    //MCU選項, 目前僅有實作ESP32
    enum{
      MCU_SUPPORT_ESP32,
      MCU_SUPPORT_ESP8266,
      MCU_SUPPORT_UNO,
      MCU_SUPPORT_NANO,
      MCU_SUPPRRT_AMEBAD
    };

    //設太多*.txt其實也沒有辦法讀, 目前先限制讀15個檔案
    enum{
      FILE_LIST_MAX_NUM = 15 
    };

    //data reader會用到的選項
    enum{
      MODE_REGRESSION,
      MODE_BINARY,
      MODE_CATEGORICAL,
    };

    Flag_DataBuffer data;

    //Funtion Name: DataReader
    //Purpose: 建構子
    //Parameter: None
    //Return: None
    Flag_DataReader(){
      memset(&data,0,sizeof(data));
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

    uint32_t getNumOfFiles(){
      return _fileNum;
    }
    //Funtion Name: read
    //Purpose: 依據所設定的路徑讀取資料.txt然後將其存入public成員data供user存取
    //Parameter: None
    //Return: None
    Flag_DataBuffer* read(String filePath, uint8_t mode, uint8_t mcu_type = MCU_SUPPORT_ESP32){
      _mcu_type = mcu_type;
      _filePath = filePath; //若是分類問題, 使用,號區隔檔案, 換行為結尾
      _mode = mode;
      
      //由於檔案系統庫不同, 所以要分MCU
      if(_mcu_type == MCU_SUPPORT_ESP32){
        
        //確認檔案系統是否work
        if(!SPIFFS.begin(true)){
          if(_debugInfoType <= INFO_BASE){
            Serial.println(F("無法掛載SPIFFS分區, 請重置"));
          }
          while(1){}  //blocking
        }
        
        //依據不同深度學習問題類型做data read的動作
        if(_mode == MODE_REGRESSION){
          uint32_t readDataLen = 0;// 資料筆數
          uint32_t featureNum = 0; // feature dim
          uint32_t labelNum = 0;   // label dim
          uint32_t stage = 0;      // 0 for feature; 1 for label; 2 for finish

          //開啟*.txt
          File file = SPIFFS.open(_filePath); // 預設為 r mode
          // Serial.print("File Name: "); Serial.println(file.name());
          // Serial.print("File Size: "); Serial.println(file.size());

          if(!file || file.isDirectory()){
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("無法開啟檔案, 請重置"));
            }
            while(1){}  //blocking
          }
          _fileNum = 1;
          uint8_t get_once = 0;
          char *fileStr  = (char *) malloc(sizeof(char) * file.size());  //用這樣才不會出事, 使用string會非常佔heap; 另外使用stack, 若資料量太大, 也無法完整存入
          uint32_t tempCnt = 0;

          //先得到檔案字串, featureNum, labelNum, readDataLen
          while(file.available()){
            char chr = (char)file.read();
            fileStr[tempCnt] = chr;
            tempCnt++;

            if(chr == ','){
              if(stage == 0){
                //feature stage 
                if(!get_once) featureNum++;
              }else if(stage == 1){
                //label stage
                if(!get_once) labelNum++;
              }
            }

            if(chr == ' '){
              if(!get_once) featureNum++;
              stage = 1;
            }
            
            if(chr == '\n'){
              if(!get_once) labelNum++;
              readDataLen += 1; //REGRESSION以'\n'來區別資料筆數
              stage = 2;        //finish stage
              get_once = 1;
            }
          }
          
          //有了featureNum, labelNum, readDataLen就可以進行動態記憶體配置
          data.feature = (float *) malloc(sizeof(float) * readDataLen * featureNum);  
          data.label   = (float *) malloc(sizeof(float) * readDataLen * labelNum);

          //底下解析file string的資料並寫到動態記憶體中
          stage = 0;  //0 for feature; 1 for label; 2 for finish
          uint32_t strIndex = 0;
          uint32_t dataIndex = 0;  // feature Data Arry Len
          uint32_t labelIndex = 0; // label Data Arry Len
          String tempStr="";
          
          for(int i = 0; i < readDataLen; i++){
            while(1){
              if(fileStr[strIndex] != ' ' && fileStr[strIndex] != '\n' && fileStr[strIndex] != ','){
                tempStr += fileStr[strIndex];
                strIndex++;
                
                if(fileStr[strIndex] == ',' || fileStr[strIndex] == ' '){
                  if(stage == 0){
                    //表示收齊一筆中的一個feature input
                    data.feature[dataIndex] = tempStr.toFloat();
                    tempStr="";
                    dataIndex++;
                  }else if(stage == 1){
                    //表示收齊一筆中的一個label
                    data.label[labelIndex] = tempStr.toFloat();
                    tempStr="";
                    labelIndex++;
                  }
                  if(fileStr[strIndex] == ' '){
                    stage = 1;
                  }
                  strIndex++;
                }else if(fileStr[strIndex] == '\n'){
                  //表示收齊一筆中的一個label
                  data.label[labelIndex] = tempStr.toFloat();
                  labelIndex++;

                  //表示收齊一筆data, stage歸0
                  stage = 0;
                  tempStr="";
                  strIndex++;
                  break;
                }
              }
            }
          }
          
          // 更新data資料結構
          // data.feature = ; //這在樓上就設定好了
          // data.label = ;   //這在樓上就設定好了
          data.featureDim = featureNum;
          data.labelDim = labelNum;
          data.dataLen = readDataLen;
          data.featureDataArryLen = dataIndex;
          data.labelDataArryLen = labelIndex;
          
          // 資料預處理會用到的統計數學物件
          Flag_Statistics stats;
          data.featureMean = stats.mean(data.feature, data.featureDataArryLen); // 計算特徵資料的平均值
          data.featureSd = stats.sd(data.feature, data.featureDataArryLen);     // 計算特徵資料的標準差
          data.labelMaxAbs = stats.maxAbs(data.label, data.labelDataArryLen);

          if(_debugInfoType <= INFO_SIMPLE){
            Serial.printf("資料總數: %d\n", readDataLen);
            Serial.printf("每筆Feature輸入數量: %d\n", featureNum);
            Serial.printf("每筆Label輸入數量: %d\n", labelNum);
            Serial.printf("Feature Total Len: %d\n", dataIndex);
            Serial.printf("Label Total Len: %d\n", labelIndex);
            Serial.printf("Feature平均值: %d\n", data.featureMean);
            Serial.printf("Feature標準差: %d\n", data.featureSd);
            Serial.printf("Label MAX: %d\n", data.labelMaxAbs);
            Serial.println();
          }

          if(_debugInfoType <= INFO_VERBOSE){
            Serial.println(F("檔案內容如下:"));
            for(int a = 0; a < file.size(); a++){
              Serial.print(fileStr[a]);
            }
          }

          free(fileStr); //記得一定要把file str給釋放掉
          file.close();

          if(_debugInfoType <= INFO_SIMPLE){
            Serial.print(F("讀檔後剩餘Heap : "));
            Serial.println(ESP.getFreeHeap());
          }
          
        }else if(_mode == MODE_BINARY){
          uint32_t readDataLen = 0;
          uint32_t featureNum = 0;
          uint32_t labelNum = 0;

          //先確定分類數量
          uint32_t classifyNum = 0;
          uint32_t strIndex = 0;
          
          while(1){
            if(_filePath[strIndex] == ','){
              classifyNum++; //讀到一個檔案, 新增一個分類
            }else if(_filePath[strIndex] == '\0'){
              classifyNum++; //讀到一個檔案, 新增一個分類
              break;         //解析完畢
            }
            strIndex++;
          }
          
          //2元分類中, 若發現分類數量(檔案數量)不等於2個則代表設置錯誤
          if(classifyNum != 2){
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("檔案路徑設置錯誤 or 深度學習類型設置錯誤, 請重置"));
            }
            while(1){}  //blocking
          }
          _fileNum = 2;
          //準備fileList存檔名
          //String *fileList = (String *)malloc(sizeof(String) * classifyNum); //多元分類才需要這樣做
          String fileList[2];
          String tempStr="";
          strIndex = 0;
          uint8_t fileIndex = 0;

          while(1){
            tempStr += _filePath[strIndex];
            strIndex++;

            if(_filePath[strIndex] == ','){
              fileList[fileIndex] = tempStr;
              tempStr = "";
              fileIndex++;
              strIndex++;
            }else if(_filePath[strIndex] == '\0'){
              fileList[fileIndex] = tempStr;
              tempStr = "";
              fileIndex++;
              strIndex++;
              break;      //解析完畢
            }
          }

          //知道檔名後讀個別的檔案字串, featureNum, labelNum, readDataLen
          for(int i = 0; i < classifyNum; i++){
            //開啟*.txt
            File file = SPIFFS.open(fileList[i]); // default r mode
            // Serial.print("File Name: "); Serial.println(file.name());
            // Serial.print("File Size: "); Serial.println(file.size());

            if(!file || file.isDirectory()){
              if(_debugInfoType <= INFO_BASE){
                Serial.println(F("無法開啟檔案, 請重置"));
              }
              while(1){}  //blocking
            }

            readDataLen = 0;
            featureNum = 0;
            labelNum = 0;
            uint8_t getOnce = 0;

            char *fileStr  = (char *) malloc(sizeof(char) * file.size());  

            uint32_t tempCnt = 0;
            while(file.available()){
              char chr = (char)file.read();
              fileStr[tempCnt]= chr;
              tempCnt++;

              if(chr == ','){
                if(!getOnce) featureNum++;
              }

              if(chr == '\n'){  
                if(!getOnce) featureNum++; //一行之中只會有feature input 因為二元分類的label是以檔名區分
                if(!getOnce) labelNum++;   //其實二元分類一行之中固定只有一個labelNum, 其值要不是0就是1
                readDataLen += 1;          //BINARY以'\n'來區別資料筆數
                getOnce = 1;
              }
            }
            
            //有了featureNum, labelNum, readDataLen就可以進行動態記憶體配置
            _binaryData.file[i].feature  = (float *) malloc(sizeof(float) * readDataLen * featureNum);  
            _binaryData.file[i].label    = (float *) malloc(sizeof(float) * readDataLen * labelNum);

            //底下解析file string的資料並寫到動態記憶體中
            uint32_t strIndex = 0;
            uint32_t dataIndex = 0;
            uint32_t labelIndex = 0;
            String tempStr="";
        
            for(int j = 0; j < readDataLen; j++){
              while(1){
                if(fileStr[strIndex] != '\n' && fileStr[strIndex]!= ','){
                  tempStr += fileStr[strIndex];
                  strIndex++;
                  
                  if(fileStr[strIndex] == ','){
                    //表示收齊一筆中的一個feature input
                    _binaryData.file[i].feature[dataIndex] = tempStr.toFloat();
                    tempStr="";
                    dataIndex++;
                    strIndex++;
                  }else if(fileStr[strIndex]== '\n'){
                    //表示收齊一筆中的一個feature input
                    _binaryData.file[i].feature[dataIndex] = tempStr.toFloat();
                    tempStr="";
                    dataIndex++;

                    //表示收齊一筆中的一個label
                    _binaryData.file[i].label[labelIndex] = i; //label 的值 就是檔案索引
                    labelIndex++;

                    strIndex++;
                    break; //表示收齊一筆data
                  }
                }
              }
            }

            //更新_binaryData資料結構
            //_binaryData.file[].feature = ;
            //_binaryData.file[].label = ;
            _binaryData.file[i].featureDim = featureNum;
            _binaryData.file[i].labelDim = labelNum;
            _binaryData.file[i].dataLen = readDataLen;
            _binaryData.file[i].featureDataArryLen = dataIndex;
            _binaryData.file[i].labelDataArryLen = labelIndex;

            if(_debugInfoType <= INFO_SIMPLE){
              Serial.printf("----File%d----\n", i+1);
              Serial.printf("資料總數: %d\n", readDataLen);
              Serial.printf("每筆Feature輸入數量: %d\n", featureNum);
              Serial.printf("每筆Label輸入數量: %d\n", labelNum);
              Serial.printf("Feature Total Len: %d\n", dataIndex);
              Serial.printf("Label Total Len: %d\n\n", labelIndex);
            }
            
            if(_debugInfoType <= INFO_VERBOSE){
              Serial.println(F("檔案內容如下:"));
              for(int a = 0; a<file.size(); a++){
                Serial.print(fileStr[a]);
              }
              Serial.println(F(""));
            }
            
            //label 僅有兩類 使用0與1即可, 所以不用特別做one-hot encode
            free(fileStr);//記得一定要把file str給釋放掉
            file.close();
          } //end of file analyze

          data.featureDim = featureNum; //file[0] & file[1] must to be equal
          data.labelDim   = labelNum;   //file[0] & file[1] must to be equal
          data.dataLen = _binaryData.file[0].dataLen + _binaryData.file[1].dataLen ;
          data.featureDataArryLen = _binaryData.file[0].featureDataArryLen + _binaryData.file[1].featureDataArryLen;
          data.labelDataArryLen   = _binaryData.file[0].labelDataArryLen   + _binaryData.file[1].labelDataArryLen;

          // if(_debugInfoType <= INFO_SIMPLE){
          //     Serial.print("剩餘Heap : ");
          //     Serial.println(ESP.getFreeHeap());
          // }

          //若feature不夠放...  
          if(data.featureDataArryLen * sizeof(float) > heap_caps_get_largest_free_block(MALLOC_CAP_32BIT)){
            if(_debugInfoType <= INFO_BASE){
              Serial.print(F("目前能分配出最大的動態記憶體區段為"));
              Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
              Serial.println(F("B"));
              Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
            }
            while(1);
          }

          //若label不夠放...  
          if(data.labelDataArryLen * sizeof(float) > heap_caps_get_largest_free_block(MALLOC_CAP_32BIT)){
            if(_debugInfoType <= INFO_BASE){
              Serial.print(F("目前能分配出最大的動態記憶體區段為"));
              Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
              Serial.println(F("B"));
              Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
            }
            while(1);
          }
          //combine two file to one data set
          data.feature = (float *) calloc(data.featureDataArryLen, sizeof(float));
          data.label   = (float *) calloc(data.labelDataArryLen,  sizeof(float));

          // if(_debugInfoType <= INFO_SIMPLE){
          //     Serial.print("剩餘Heap : ");
          //     Serial.println(ESP.getFreeHeap());
          // }

          if(_debugInfoType <= INFO_SIMPLE){
            Serial.printf("----Total----");
            Serial.printf("資料總數: %d\n", data.dataLen);
            Serial.printf("每筆Feature輸入數量: %d\n", data.featureDim);
            Serial.printf("每筆Label輸入數量: %d\n", data.labelDim);
            Serial.printf("Feature Total Len: %d\n", data.featureDataArryLen);
            Serial.printf("Label Total Len: %d\n", data.labelDataArryLen);
          }

          //in fact, in binary mode, classifyNum == 2 
          for(int q = 0; q < classifyNum; q++){
            for(int k = 0; k < _binaryData.file[q].featureDataArryLen; k++){
              data.feature[k + q * _binaryData.file[0].featureDataArryLen] = _binaryData.file[q].feature[k];
            }  
            for(int y = 0; y < _binaryData.file[q].labelDataArryLen; y++){
              data.label[y +  q * _binaryData.file[0].labelDataArryLen] = _binaryData.file[q].label[y];
            }
            free(_binaryData.file[q].feature);
            free(_binaryData.file[q].label);
          }

          // 資料預處理會用到的統計數學物件
          Flag_Statistics stats;
          data.featureMean = stats.mean(data.feature, data.featureDataArryLen); // 計算特徵資料的平均值
          data.featureSd = stats.sd(data.feature, data.featureDataArryLen);     // 計算特徵資料的標準差
          data.labelMaxAbs = 0; //2元分類不用labelMaxAbs

          if(_debugInfoType <= INFO_SIMPLE){
            Serial.printf("Feature平均值: %d\n", data.featureMean);
            Serial.printf("Feature標準差: %d\n", data.featureSd);
            Serial.println();
          }
            
          if(_debugInfoType <= INFO_SIMPLE){
              Serial.print(F("讀檔後剩餘Heap : "));
              Serial.println(ESP.getFreeHeap());
          }

        }else if(_mode == MODE_CATEGORICAL){
          uint32_t readDataLen = 0;
          uint32_t featureNum = 0;
          uint32_t labelNum = 0;

          //先確定分類數量
          uint32_t classifyNum = 0;
          uint32_t strIndex = 0;
          
          while(1){
            if(_filePath[strIndex] == ','){
              classifyNum++; //讀到一個檔案, 新增一個分類
            }else if(_filePath[strIndex] == '\0'){
              classifyNum++; //讀到一個檔案, 新增一個分類
              break;         //解析完畢
            }
            strIndex++;
          }
          
          //多元分類中, 若發現分類數量(檔案數量)<=2個則代表設置錯誤
          if(classifyNum <= 2){
            if(_debugInfoType <= INFO_BASE){
              Serial.println(F("檔案路徑設置錯誤 or 深度學習類型設置錯誤, 請重置"));
            }
            while(1){}  //blocking
          }
          _fileNum = classifyNum;
          
          //準備fileList存檔名
          //String *fileList = (String *)malloc(sizeof(String) * classifyNum); //再研究 這樣會當掉
          String fileList[FILE_LIST_MAX_NUM];
          String tempStr="";
          strIndex = 0;
          uint8_t fileIndex = 0;
          
          while(1){
            tempStr += _filePath[strIndex];
            strIndex++;

            if(_filePath[strIndex] == ','){
              fileList[fileIndex] = tempStr;
              tempStr = "";
              fileIndex++;
              strIndex++;
            }else if(_filePath[strIndex] == '\0'){
              fileList[fileIndex] = tempStr;
              tempStr = "";
              fileIndex++;
              strIndex++;
              break;      //解析完畢
            }
          }
            
          //知道檔名後讀個別的檔案字串, featureNum, labelNum, readDataLen
          for(int i = 0; i < classifyNum; i++){
            //開啟*.txt
            File file = SPIFFS.open(fileList[i]); // default r mode
            // Serial.print("File Name: "); Serial.println(file.name());
            // Serial.print("File Size: "); Serial.println(file.size());
            
            if(!file || file.isDirectory()){
              if(_debugInfoType <= INFO_BASE){
                Serial.println(F("無法開啟檔案, 請重置"));
              }
              while(1){}  //blocking
            }

            readDataLen = 0;
            featureNum = 0;
            labelNum = 0;
            uint8_t getOnce = 0;

            char *fileStr  = (char *) malloc(sizeof(char) * file.size());  

            uint32_t tempCnt = 0;
            while(file.available()){
              char chr = (char)file.read();
              fileStr[tempCnt]= chr;
              tempCnt++;

              if(chr == ','){
                if(!getOnce) featureNum++;
              }

              if(chr == '\n'){  
                if(!getOnce) featureNum++; //一行之中只會有feature input 因為categorical的label是以檔名區分
                //if(!getOnce) labelNum++; //多元分類的labelNum跟二元分類的算法不同, 多元分類的labelNum等於classifyNum
                readDataLen += 1;          //categorical以'\n'來區別資料筆數
                getOnce = 1;
              }
            }
            
            //有了featureNum, labelNum, readDataLen就可以進行動態記憶體配置
            _categoricalData.file[i].feature  = (float *) malloc(sizeof(float) * readDataLen * featureNum);  
            //_categoricalData.file[i].label  = (float *) malloc(sizeof(float) * readDataLen * labelNum);  //多元分類不是這樣
            _categoricalData.file[i].label  = (float *) malloc(sizeof(float) * readDataLen * classifyNum); //而是這樣

            //底下解析file string的資料並寫到動態記憶體中
            uint32_t strIndex = 0;
            uint32_t dataIndex = 0;
            uint32_t labelIndex = 0;
            String tempStr="";
            
            for(int j = 0; j < readDataLen; j++){
              while(1){
                if(fileStr[strIndex] != '\n' && fileStr[strIndex]!= ','){
                  tempStr += fileStr[strIndex];
                  strIndex++;
                  
                  if(fileStr[strIndex] == ','){
                    //表示收齊一筆中的一個feature input
                    _categoricalData.file[i].feature[dataIndex] = tempStr.toFloat();
                    tempStr="";
                    dataIndex++;
                    strIndex++;
                  }else if(fileStr[strIndex]== '\n'){
                    //表示收齊一筆中的一個feature input
                    _categoricalData.file[i].feature[dataIndex] = tempStr.toFloat();
                    tempStr="";
                    dataIndex++;

                    //表示收齊一筆中的一個label, 要做one-hot encoding
                    for(int k = 0; k < classifyNum; k++){
                      if(k == i){
                        _categoricalData.file[i].label[labelIndex] = 1;
                      }else{
                        _categoricalData.file[i].label[labelIndex] = 0;
                      }
                      labelIndex++;
                    }

                    strIndex++;
                    break; //表示收齊一筆data
                  }
                }
              }
            }
            
            //更新_categoricalData資料結構
            //_categoricalData.file[].feature = ;
            //_categoricalData.file[].label = ;
            _categoricalData.file[i].featureDim = featureNum;
            //_categoricalData.file[i].labelDim = labelNum; //多元分類不是這樣
            _categoricalData.file[i].labelDim = classifyNum;//而是這樣
            _categoricalData.file[i].dataLen = readDataLen;
            _categoricalData.file[i].featureDataArryLen = dataIndex;
            _categoricalData.file[i].labelDataArryLen = labelIndex;

            if(_debugInfoType <= INFO_SIMPLE){
              Serial.printf("----File%d----\n", i+1);
              Serial.printf("資料總數: %d\n", readDataLen);
              Serial.printf("每筆Feature輸入數量: %d\n", featureNum);
              //Serial.printf("每筆Label輸入數量: %d\n", labelNum);  //多元分類不是這樣
              Serial.printf("每筆Label輸入數量: %d\n", classifyNum); //而是這樣
              Serial.printf("Feature Total Len: %d\n", dataIndex);
              Serial.printf("Label Total Len: %d\n\n", labelIndex);
            }
            
            if(_debugInfoType <= INFO_VERBOSE){
              Serial.println(F("檔案內容如下:"));
              for(int a = 0; a<file.size(); a++){
                Serial.print(fileStr[a]);
              }
            }
            
            //label 僅有兩類 使用0與1即可, 所以不用特別做one-hot encode
            free(fileStr);//記得一定要把file str給釋放掉
            file.close();
          } //end of file analyze

          data.featureDim = featureNum; //files must equal
          //data.labelDim = labelNum;   //多元分類不是這樣
          data.labelDim = classifyNum;  //而是這樣

          data.dataLen = 0;
          data.featureDataArryLen = 0;
          data.labelDataArryLen = 0;
          for(int i = 0; i < classifyNum; i++){
            data.dataLen += _categoricalData.file[i].dataLen;
            data.featureDataArryLen += _categoricalData.file[i].featureDataArryLen;
            data.labelDataArryLen += _categoricalData.file[i].labelDataArryLen;
          }

          // if(_debugInfoType <= INFO_SIMPLE){
          //     Serial.print("剩餘Heap : ");
          //     Serial.println(ESP.getFreeHeap());
          // }
          
          //若feature分配不出來...
          if(data.featureDataArryLen * sizeof(float) > heap_caps_get_largest_free_block(MALLOC_CAP_32BIT)){
            if(_debugInfoType <= INFO_BASE){
              Serial.print(F("目前能分配出最大的動態記憶體區段為"));
              Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
              Serial.println(F("B"));
              Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
            }
            while(1);
          }

          //若label分配不出來...
          if(data.labelDataArryLen * sizeof(float) > heap_caps_get_largest_free_block(MALLOC_CAP_32BIT)){
            if(_debugInfoType <= INFO_BASE){
              Serial.print(F("目前能分配出最大的動態記憶體區段為"));
              Serial.print(heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
              Serial.println(F("B"));
              Serial.println(F("Heap分配不出合適的記憶體片段, 請重置"));
            }
            while(1);
          }

          //combine two file to one data set
          data.feature = (float *) calloc(data.featureDataArryLen, sizeof(float));
          data.label   = (float *) calloc(data.labelDataArryLen, sizeof(float));

          // if(_debugInfoType <= INFO_SIMPLE){
          //     Serial.print("剩餘Heap : ");
          //     Serial.println(ESP.getFreeHeap());
          // }

          if(_debugInfoType <= INFO_SIMPLE){
            Serial.printf("----Total----");
            Serial.printf("資料總數: %d\n", data.dataLen);
            Serial.printf("每筆Feature輸入數量: %d\n", data.featureDim);
            Serial.printf("每筆Label輸入數量: %d\n", data.labelDim);
            Serial.printf("Feature Total Len: %d\n", data.featureDataArryLen);
            Serial.printf("Label Total Len: %d\n", data.labelDataArryLen);
          }

          for(int q = 0; q < classifyNum; q++){
            uint32_t featureAryLen = 0;
            uint32_t labelAryLen = 0;
            //accumulate array len
            for(int x = 0; x < q; x++){
              featureAryLen += _categoricalData.file[q].featureDataArryLen;
              labelAryLen += _categoricalData.file[q].labelDataArryLen;
            }
            for(int k = 0; k < _categoricalData.file[q].featureDataArryLen; k++){
              data.feature[k + featureAryLen] = _categoricalData.file[q].feature[k];
            }  
            for(int y = 0; y < _categoricalData.file[q].labelDataArryLen; y++){
              data.label[y + labelAryLen] = _categoricalData.file[q].label[y];
            }
            free(_categoricalData.file[q].feature);
            free(_categoricalData.file[q].label);
          }

          // 資料預處理會用到的統計數學物件
          Flag_Statistics stats;
          data.featureMean = stats.mean(data.feature, data.featureDataArryLen); // 計算特徵資料的平均值
          data.featureSd = stats.sd(data.feature, data.featureDataArryLen);     // 計算特徵資料的標準差
          data.labelMaxAbs = 0; //多元分類不用labelMaxAbs

          if(_debugInfoType <= INFO_SIMPLE){
            Serial.printf("Feature平均值: %d\n", data.featureMean);
            Serial.printf("Feature標準差: %d\n", data.featureSd);
            Serial.println();
          }

          if(_debugInfoType <= INFO_SIMPLE){
              Serial.print(F("讀檔後剩餘Heap : "));
              Serial.println(ESP.getFreeHeap());
          }

        }else{
          if(_debugInfoType <= INFO_BASE){
              Serial.println(" 深度學習類型設置錯誤, 請重置");
          }
          while(1){}  //blocking
        }
        
      }else if(_mcu_type == MCU_SUPPORT_ESP8266){
        //TODO
      }else if(_mcu_type == MCU_SUPPORT_UNO){
        //TODO
      }else if(_mcu_type == MCU_SUPPORT_NANO){
        //TODO
      }else if(_mcu_type == MCU_SUPPRRT_AMEBAD){
        //TODO
      }

      return &(this->data);
    }
    
    void freeMemory(){
      //使用malloc則要記得釋放
      free(data.feature);
      free(data.label);
    }

  private:
    String _filePath;
    uint8_t _debugInfoType;
    uint8_t _mode;
    uint8_t _mcu_type;
    uint32_t _fileNum;

    struct _binaryData_t{
      Flag_DataBuffer file[2];
    }_binaryData;

    struct _categoricalData_t{
      Flag_DataBuffer file[FILE_LIST_MAX_NUM];
    }_categoricalData;
};

#endif