#include <SPI.h>
#include <SD.h>

const int chipSelect = 4;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for Serial to initialize (for boards like Leonardo)
  }

  Serial.println("Initializing SD card...");

  // Check if the SD card module is detected
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized successfully!");

  // Test by creating a file
  File testFile = SD.open("test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("Hello, SD card!");
    testFile.close();
    Serial.println("File created and written successfully!");
  } else {
    Serial.println("Error creating file!");
  }

  // Check if the file can be read
  testFile = SD.open("test.txt");
  if (testFile) {
    Serial.println("Reading file contents:");
    while (testFile.available()) {
      Serial.write(testFile.read());
    }
    testFile.close();
    Serial.println("\nFile read successfully!");
  } else {
    Serial.println("Error reading file!");
  }
}

void loop() {
  // Nothing to do here
}
