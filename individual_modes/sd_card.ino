/*
  SD card read/write

  This example shows how to read and write data to and from an SD card file
  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

  created   Nov 2010
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe

  This example code is in the public domain.

*/

#include <SPI.h>
#include <SD.h>
File myFile;

void setup() {
  Serial.begin(9600); // Open serial communications and wait for port to open:
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("test.txt", FILE_WRITE);

  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3.");
    myFile.close(); // close the file:
    Serial.println("done.");
  } else 
    Serial.println("error opening test.txt"); // if the file didn't open, print an error:

  myFile = SD.open("test.txt"); // re-open the file for reading:
  if (myFile) {
    Serial.println("test.txt:");
    while (myFile.available()) Serial.write(myFile.read());
    myFile.close(); // close the file:
  } else 
    Serial.println("error opening test.txt"); // if the file didn't open, print an error:
}

void loop() {
  // nothing happens after setup
}
