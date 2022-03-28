/*
  讓蜂鳴器發出聲音
*/
#define BUZZER_PIN 32

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  digitalWrite(BUZZER_PIN, HIGH);  
  delay(300);                       
  digitalWrite(BUZZER_PIN, LOW);    
  delay(1000);                   
}