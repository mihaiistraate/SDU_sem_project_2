#include <SPI.h> //library for sd card module
#include <SD.h> //library for sd card module
#include "SHT85.h" //library for temp+humidity sensor
#define heartPin A1 //heart rate sensor input pin
#define tempPin A0 //temperature sensor input pin
#define ledPin 2 //led output pin
#define SHT85_ADDRESS 0x44 //i2c adress of the temp+humidity sensor
#define sample_size 134 //this gaves at between 2 and 7 peaks when the heart rate is between [45, 170] with a 20ms reading delay
#define V_REF 5

int guard = 0;
long int cnt; //variable to keep track of time when measuring stress
float heartrate = 0, temp = 0, humidity = 0, temp_sensor = 0, stress_score = 50, ref_values[4];
//the starting point for the stress score is 50, the range is [0, 100]
float ref_time;

SHT85 sht;
File myFile;

void blink_led(int n, int delay_time){
  for (int i=0; i<n; ++i){ //turn off led for 500ms and then on for a specified number of ms n times
    digitalWrite(ledPin, LOW);
    delay(500);
    digitalWrite(ledPin, HIGH);
    delay(delay_time);
  }
}

float get_voltage(int port){
  int sensorValue = 0;
  for (int i = 0; i < 20 ; ++i){ //calculating the average analog voltage by taking 20 measurments
    sensorValue += analogRead(port);
    delay(5);
  }
  return sensorValue * V_REF / (1023.00 * 20.00); // Convert to voltage and return the value
}

float calculate_temperature(){
  return get_voltage(tempPin) * 100.0; //converting to Degree Celsius
}

void avg_values(int n){ //function that calculates the average values of the parameters across a given timeframe
  int cnt_tempsens = 0, cnt_temp = 0, cnt_humidity = 0, cnt_heart = 0; //count the amount of valid reading
  float sum_tempsens = 0, sum_temp = 0, sum_humidity = 0, sum_heart = 0; //store the sum of valid readings
  int data[sample_size]; //array for storing the analog readings of the heart rate sensor

  for(int i=1; i<=n; ++i){
    for(int j=0; j<sample_size; ++j){ //read the analog values for the heart rate sensor
      data[j]=analogRead(heartPin);
      delay(20); //have a delay of 20ms between readings
    }
    for(int j=0; j<10; ++j){ //read values from the other sensors over approx 1s
      float tempsens_reading=calculate_temperature();
      sht.read(); // default = true/fast  slow = false
      float temp_reading=sht.getTemperature();
      float humidity_reading=sht.getHumidity();
    
      if (tempsens_reading >= 20 && tempsens_reading <= 42){ //varify if the readings are valid
        sum_tempsens+=tempsens_reading;
        ++cnt_tempsens;
      }
      if (temp_reading >= 20 && temp_reading <= 42){
        sum_temp+=temp_reading;
        ++cnt_temp;
      }
      if (humidity_reading >= 0 && humidity_reading <= 100){
        sum_humidity+=humidity_reading;
        ++cnt_humidity;
      }
      delay(100);
    }
    // Perform autocorrelation of the analog values given by the heart rate sensor to find the frequency(beat)
    unsigned int bestLag;
    float minCorrelation = 1000000;

    for(unsigned int lag = 15; lag < (sample_size / 2 ); lag++){
      float correlation = 0;
      unsigned int count = 0;

      for(unsigned int i = 0; i < sample_size - lag; i++){
        correlation += (data[i] - data[i + lag]) * (data[i] - data[i + lag]);
        count++;
      }

      correlation /= count;
      if (correlation < minCorrelation){
         minCorrelation = correlation;
        bestLag = lag;
      }
    }
    
    int heart_reading = 3000 / bestLag; //calculate the frequency in beats/min (1/period)
    if(heart_reading>=45 && heart_reading<=170){ //verify is the reading is valid
      sum_heart+=heart_reading;
      ++cnt_heart;
    }
    else if(n == 1) sum_heart = -1;
  }
  
  if(sum_heart == -1) heartrate=ref_values[0];
  else heartrate = sum_heart/cnt_heart; //store the average readings in variables
  temp_sensor = 1.00*sum_tempsens/cnt_tempsens;
  temp = 1.00*sum_temp/cnt_temp;
  humidity = 1.00*sum_humidity/cnt_humidity;
}

void extract_numbers(const char* inputString) {
  char tempNumber[10]; // Temporary array to store characters forming a number
  int tempIndex = 0, cnt_var = 1; // Index for the temporary array

  for (int i = 0; i < strlen(inputString); i++) {
    char c = inputString[i];
    if (isdigit(c) || c == '.'){ // Check if the character is a digit or a comma
      tempNumber[tempIndex++] = c; // Add the digit or comma to the temporary array
    } else if (c == ',' || i == strlen(inputString) - 1) {
      // If the character is a comma or it's the last character of the string,
      // convert the temporary array to a number and print it
      if (tempIndex > 0) {
        tempNumber[tempIndex] = '\0'; // Terminate the temporary array with null character
        float number = atof(tempNumber); // Convert the temporary array to a float
        if(cnt_var == 2) ref_values[0]=number;
        if(cnt_var == 3) ref_values[1]=number;
        if(cnt_var == 4) ref_values[2]=number;
        if(cnt_var == 5) ref_values[3]=number;
        ++cnt_var;
        tempIndex = 0; // Reset the index for the next number
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(tempPin, INPUT); //configuring sensor pin as input for temperature sensor
  pinMode(ledPin, OUTPUT); //configuring led pin as output

  Wire.begin(); //configuration lines for the temp+humidity sensor
  sht.begin(SHT85_ADDRESS);
  Wire.setClock(100000);

  if (!SD.begin(4)){ //sd card is connected to D4
    digitalWrite(ledPin, LOW); //keep the led off until the module works
    while (1); //only quit the while if the sd card works
  }
  myFile = SD.open("res.csv", FILE_READ); //open the file in reading mode(starts from the beginning of the file). Only one file can be opened at a time
  if (!myFile){ //if the file doesn't exist
    digitalWrite(ledPin, LOW); //keep the led off
    while(1); //the device has to be restarted
  }
  else{ //the file exists on the sd card
    ref_values[0]=-1; //initialise this to -1 to verify later if it has been updated to a refrence value
    char line[100]; //lines are not longer than 100 characters
    int cnt_line = 1; //keeping track of what line has been read on the SD file
    while (myFile.available()){ //reading the SD card file
      memset(line, 0, sizeof(line)); //reset to all elements to 0 (null characters) in the array after each reading
      myFile.readBytesUntil('\n', line, sizeof(line)); //read until newline character
      if(cnt_line==2) extract_numbers(line);
      ++cnt_line; //keeping track of the lines read
    }
    myFile.close();
    if(ref_values[0]==-1){ //go through a calibration process if nothing has been found when reading the file
      blink_led(4, 1000); //blink the led when the calibration process begins and then keep it on
      avg_values(25); //25*4500(delay)=112500=approx2min
      ref_values[0] = heartrate; //store the calculated refrence values in an array
      ref_values[1] = temp_sensor; //sensor on the arm
      ref_values[2] = temp;
      ref_values[3] = humidity;

      myFile = SD.open("res.csv", FILE_WRITE); //reopen the file in writing mode(starts from the end to allow writing)
      myFile.println("Time,Stress score,Heart rate,Temperature arm,Temperature chest,Humidity"); //printing column names in the file
      myFile.print("Ref,");
      myFile.print(stress_score, 2);
      myFile.print(",");
      myFile.print(ref_values[0], 0);
      myFile.print(",");
      myFile.print(ref_values[1], 2);
      myFile.print(",");
      myFile.print(ref_values[2], 2); //2 means the number of decimals
      myFile.print(",");
      myFile.println(ref_values[3], 2);
      myFile.close();
    }
  }
  blink_led(6, 500); //blink the led faster and for more times when the stress measurement begins and then keep it on
}

float normalise_value(float n, float maxim){ //x_norm=(x-x_min)/(x_max-x_min) - for any x
  return 1.00*n/maxim; //always consider min=0
}

void print(){
  cnt=millis()-ref_time; //calculating the time since the stress measurement began in msec
  if(cnt>=60000){ //if it exceeeds 1 min, display it is min and sec
    int min=cnt/60000; //calculate min
    myFile.print(min);
    myFile.print(" min and ");
    myFile.print(((cnt - min*60000) % 60000)/1000.00, 2); //calculate sec
    myFile.print(" sec,");
  }
  else{ //there are only seconds
    myFile.print(cnt/1000.00, 2); //calculate sec from msec
    myFile.print(",");
  }
  myFile.print(stress_score, 2);
  myFile.print(",");
  myFile.print(heartrate, 0);
  myFile.print(",");
  myFile.print(temp_sensor, 2); //printing the temperature in the file
  myFile.print(",");
  myFile.print(temp, 2); //2 means the number of decimals
  myFile.print(",");
  myFile.println(humidity, 2);
}

void loop(){
  avg_values(1); //come up with new readings aprox. every 4.5s
  //formula to calculate stress score
  stress_score += 50*(0.7*normalise_value((heartrate-ref_values[0]), 80)+0.2*normalise_value((temp-temp_sensor-(ref_values[2]-ref_values[1])), 3)+0.1*normalise_value((humidity-ref_values[3]), 100));
  if(stress_score>100) stress_score=100;
  if(stress_score<0) stress_score=0;
  ref_values[0]=heartrate; //compare the current values to the ones measured before
  ref_values[1]=temp_sensor;
  ref_values[2]=temp;
  ref_values[3]=humidity;

  myFile = SD.open("res.csv", FILE_WRITE); //reopen the file in writing mode(starts from the end to allow writing)
  if(!guard){ //store the time when the device starts measuring stress (after the calibration phase)
    ref_time=millis();
    guard = 1; //never go through this if again
  }

  if (!myFile){ //if the SD card has been removed, stop the code here
    digitalWrite(ledPin, LOW); //keep the led off
    while(1); //the device has to be restarted
  }
  
  print(); //print the values on the sd card
  myFile.close();
}
