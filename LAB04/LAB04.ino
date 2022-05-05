/*
  電子秤 -- 蒐集訓練資料
*/
#include <Flag_HX711.h>

#define DATA_LEN 10

// ------------全域變數------------
// 感測器的物件
Flag_HX711 hx711(32, 33); // SCK, DT

// 特徵 - 標籤資料表, 用於匯出檔案內容
float dataTable[DATA_LEN][2];
// -------------------------------

void setup() {
  // 序列埠設置
  Serial.begin(115200);
  
  // HX711 初始化
  hx711.begin();

  // 扣重調整
  hx711.tare();  

  // -------------------- 蒐集資料 --------------------
  for(int i = 0; i < DATA_LEN; i++){
    // 放置物件與記錄標準電子秤測量到的重量 (標籤資料)
    Serial.print("請在電子秤模組上放置物件, ");
    Serial.print("並輸入標準電子秤測量到的重量 (標籤): ");
    
    while(!Serial.available()); // 等待使用者輸入
    String str = Serial.readStringUntil('\n');
    dataTable[i][1] = str.toFloat();
    Serial.println(dataTable[i][1], 1);

    // 記錄電子秤測量到的重量 (特徵資料)
    dataTable[i][0] = hx711.getWeight();
    Serial.print("電子秤模組測量到的重量 (特徵): ");
    Serial.println(dataTable[i][0], 1); 

    Serial.print("蒐集了 ");
    Serial.print(i + 1);
    Serial.println(" 筆資料\n");   
  }
  Serial.println();
  Serial.println(
    "蒐集完畢, 請複製下列特徵資料與標籤資料並存成 txt 檔案"
  );
  for(int i = 0; i < DATA_LEN; i++){
    Serial.print(dataTable[i][0]);
    Serial.print(" ");
    Serial.println(dataTable[i][1]);
  }
}

void loop() {}
