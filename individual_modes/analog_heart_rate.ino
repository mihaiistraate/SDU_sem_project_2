#define sample_size 134

int data[sample_size];

void setup(){
  Serial.begin(9600);
}

void loop() {
  for (int i = 0; i < sample_size; i++) {
    data[i] = analogRead(A1);
    delay(20);  // Simulate the delay between readings (20ms in this example)
  }

  // Perform autocorrelation
  unsigned int bestLag;
  float minCorrelation = 1000000;

  for (unsigned int lag = 10; lag < (sample_size / 2 ); lag++) {
    float correlation = 0;
    unsigned int count = 0;

    for (unsigned int i = 0; i < sample_size - lag; i++){
      correlation += (data[i] - data[i + lag]) * (data[i] - data[i + lag]);
      count++;
    }

    correlation /= count;
    if (correlation < minCorrelation) {
      minCorrelation = correlation;
      bestLag = lag;
    }
  }
  // Calculate the frequency
  int frequency = 3000 / bestLag; //express it in beats/min
  Serial.println(frequency);
}
