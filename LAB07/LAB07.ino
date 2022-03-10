/*
  電子料理秤 -- 蒐集訓練資料
*/
#include <Flag_HX711.h>

#define FEATURE_TOTAL_MAX 100
#define HX711_DT_PIN_NUM 33
#define HX711_SCK_PIN_NUM 32

// ------------全域變數------------
// 感測器的物件
Flag_HX711 hx711(HX711_SCK_PIN_NUM, HX711_DT_PIN_NUM);

// 蒐集資料會用到的參數
uint8_t dataCount = 0;
// -------------------------------

void setup() {
  // UART設置
  Serial.begin(115200);
  
  // hx711設置
  hx711.begin();
  hx711.tare();  // 歸零調整
  Serial.print("offset : ");
  Serial.println(hx711.getOffset());

  Serial.println("---蒐集電子秤特徵資料與標籤資料---"); 
  Serial.println("請輸入實際重量作為標籤資料, 並更換被秤物後傳送任意字串, 來進行每一輪的資料採集");
  Serial.println("特徵資料  標籤資料");
}

void loop() {
  // 顯示重量
  Serial.print(hx711.getWeight(), 1);
  Serial.print(" ");

  // 等待使用者輸入實際重量作為標籤資料用
  while(!Serial.available());

  while(Serial.available()){
    String str = Serial.readStringUntil('\n');
    float label = str.toFloat();
    Serial.print(label);
    Serial.println();
  }

  // 等待使用者輸入任意值, 目的是為了讓使用者有空檔換用來被秤的物件
  while(!Serial.available());
    
  while(Serial.available()){
    String str = Serial.readStringUntil('\n');
  }

  dataCount++;

  if(dataCount >= FEATURE_TOTAL_MAX){
    Serial.print("已收集了");
    Serial.print(FEATURE_TOTAL_MAX);
    Serial.print("筆資料, 不用在收集了, 請複製特徵資料與標籤資料並存成txt檔案");
    while(1);
  }
}
