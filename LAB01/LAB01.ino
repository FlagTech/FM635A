/*
  閃爍 LED
*/
// setup 函式為開發板通電或每次按下 reset 按鈕後, 只會執行一次
void setup() {
  // 初始化內建 LED 腳位為輸出腳位
  pinMode(LED_BUILTIN, OUTPUT);
}
// loop 函式會不斷一直重複執行
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // 熄滅 LED(將腳位設為高電位)
  delay(1000);                      // 暫停 1 秒鐘 
  digitalWrite(LED_BUILTIN, LOW);   // 點亮 LED(將腳位設為低電位)
  delay(1000);                      // 暫停 1 秒鐘 
}