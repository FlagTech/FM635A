/*
  讓蜂鳴器發出聲音
*/
void setup() {
  // 腳位設置
  pinMode(32, OUTPUT);
}

void loop() {
  digitalWrite(32, HIGH);  
  delay(300);                       
  digitalWrite(32, LOW);    
  delay(1000);                   
}