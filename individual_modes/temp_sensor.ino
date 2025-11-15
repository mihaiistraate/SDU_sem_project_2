const int TEMPERATURE = A0; //temperature sensor input pin
const float V_REF = 5; //reference voltage level

void setup() {
  Serial.begin(9600);
  pinMode(TEMPERATURE, INPUT); //configuring sensor pin as input for temperature sensor
}

float get_voltage(int port) {
  int sensorValue = 0;
  for (int i = 0; i < 20 ; ++i) { //calculating the average analog voltage by taking 20 measurments
    sensorValue += analogRead(port);
    delay(5);
  }
  return sensorValue * V_REF / (1023.00 * 20.00); // Convert to voltage and return the value
}

float calculate_temperature() {
  return get_voltage(TEMPERATURE) * 100.0; //converting to Degree Celsius
}

void loop() {
  Serial.print("Temperature: ");
  Serial.println(calculate_temperature()); //printing the temperature
  delay(1000);
}
