/*
  控制伺服馬達
*/
void setup()
{  
  Serial.begin(115200);
  ledcSetup(7, 50, 8);  
  ledcAttachPin(2, 7);  
}

void loop()
{
  for(int angle = 0; angle <= 180; angle++)
  {
    int value=map(angle,0,180,6.4,30.72);
    ledcWrite(7, value);
    Serial.println("角度=" + String(angle) + ",value=" + String(value));
    delay(70);    
  }
  delay(1000);
}