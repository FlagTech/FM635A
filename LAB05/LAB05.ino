/*
  蒐集電子秤特徵資料與標籤資料的範例
*/
#include <HX711.h>
#include <SPIFFS.h>

HX711 hx(16, 17, 128, -0.0011545); 

double var_offset = 0; //offset變數
double var_weight = 0; //weight變數

void setup() {
  Serial.begin(115200);
  
  // Serial monitor顯示抬頭等訊息
  Serial.println("---蒐集電子秤特徵資料與標籤資料---"); 
  
  // 取得offset平均值 (歸零調整用)
  get_average();
  hx.set_offset(var_offset);  // 歸零調整
  Serial.print("offset : ");
  Serial.println(var_offset); // 印出offset值

  
  // 準備開始秤重
  Serial.println("等待電子秤穩定...");

  uint8_t validCnt = 0; //丟掉無效值過渡用的
  while(validCnt<20){
    // 取得 offset 及 weight 平均值
    get_average();
    validCnt++;
  }

  // 電子秤已穩定, 可開始蒐集資料
  Serial.println("電子秤已穩定");
  Serial.println("請輸入實際重量作為標籤資料, 並更換被秤物後傳送任意字串, 來進行下一輪的資料採集");
  Serial.println("特徵資料    標籤資料:");
}


void loop() {
  // 取得 offset 及 weight 平均值
  get_average();
  
  // serial monitor 顯示重量
  Serial.print(var_weight, 6);
  Serial.print(" ");

  // 等待使用者輸入實際重量作為標籤資料用
  //Serial.print("請輸入實際重量");
  while(Serial.available() == 0 );

  while(Serial.available() > 0 ){
    String str = Serial.readStringUntil('\n');
    float label = str.toFloat();
    Serial.print(label);
    Serial.println();
  }

  // 等待使用者輸入任意值, 目的是為了讓使用者有空檔換用來被秤的物件
  //Serial.print("請更換被秤物件, 並傳送任意值");
  while(Serial.available() == 0 );
    
  while(Serial.available() > 0 ){
    String str = Serial.readStringUntil('\n');
  }
}

// get_average() 
// 取得 offset 及 weight 平均值
void get_average() {
   // 累計10次秤重,再取平均值
   var_offset = 0;
   var_weight = 0;
   for (byte i = 0; i < 10; i++) {
      var_offset += hx.read();
      var_weight += hx.bias_read(); 
   }
   var_offset /=10;  //取offset平均值
   var_weight /=10;  //取秤重平均值
}