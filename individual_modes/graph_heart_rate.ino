#define heartPin A1

void setup() {
  Serial.begin(9600);
}
void loop() {
  int heartValue = analogRead(heartPin);
  Serial.println(heartValue);
  delay(20);
}
