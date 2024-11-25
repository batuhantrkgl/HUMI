#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <SD.h>
#include <SmtpClient.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <HttpClient.h>

#import <Client.h>
#import <Mail.h>
#import <SMTPClient.h>

#define MAX_SENSORS 20
#define DHTPIN 2       // DHT sensor connected to digital pin 2
#define DHTTYPE DHT11  // DHT 11

DHT dht(DHTPIN, DHTTYPE);
EthernetServer server(80);
const int chipSelect = 4;                             // SD card module connected to pin 4
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // Your MAC address
IPAddress ip(192, 168, 1, 100);                       // Your IP address

// SMTP settings
const char* smtpServer = "smtp.gmail.com";  // Replace with your SMTP server
IPAddress smtpServerIp(192, 168, 0, 125);   // Replace with your SMTP server's IP
const int smtpPort = 25;                    // Replace with your SMTP server's port
EthernetClient ethClient;
SmtpClient smtp(&ethClient, smtpServerIp, smtpPort);

const char* senderEmail = "sender@example.com";
const char* recipientEmail = "recipient@example.com";

float recommendedHumidity[MAX_SENSORS] = { 60.0, 60.0, 60.0 };  // Recommended humidity for each plant
int numSensors = 0;
float calibrationOffset[MAX_SENSORS] = { 0.0 };  // Array for calibration offsets

void setup() {
  Serial.begin(9600);
  dht.begin();
  Ethernet.begin(mac, ip);
  server.begin();

  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card initialization failed!");
    return;
  }

  // Load recommended humidity and calibration from SD or set defaults
  loadConfig();
  loadCalibration();
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    handleClient(client);
  }
}

void handleClient(EthernetClient& client) {
  String currentLine = "";
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n') {
        processRequest(currentLine, client);
        currentLine = "";
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }
  client.stop();
}

void processRequest(String& requestLine, EthernetClient& client) {
  logToSD("Received request: " + requestLine);
  if (requestLine.startsWith("GET /humidity")) {
    sendHumidityData(client);
  } else if (requestLine.startsWith("GET /water")) {
    int plantNum = getPlantNumber(requestLine);
    waterPlant(plantNum);
    sendResponse(client, "Watering action initiated for plant " + String(plantNum));
  } else if (requestLine.startsWith("GET /fan")) {
    int plantNum = getPlantNumber(requestLine);
    toggleFan(plantNum);
    sendResponse(client, "Fan toggled for plant " + String(plantNum));
  } else if (requestLine.startsWith("GET /calibrate")) {
    int plantNum = getPlantNumber(requestLine);
    float offset = getCalibrationValue(requestLine);
    calibrateSensor(plantNum, offset);
    sendResponse(client, "Calibration set for plant " + String(plantNum) + ": " + String(offset));
  } else if (requestLine.startsWith("GET /plant-care-tips")) {
    sendPlantCareTips(client);
  }
}

void sendPlantCareTips(EthernetClient& client) {
  String sdData = readSDCardData();
  String geminiResponse = getGeminiResponse(sdData);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println();
  client.print(geminiResponse);
}

String readSDCardData() {
  String data = "";
  if (SD.exists("data.txt")) {
    File dataFile = SD.open("data.txt");
    if (dataFile) {
      while (dataFile.available()) {
        data += char(dataFile.read());
      }
      dataFile.close();
    }
  }
  return data;
}

String getGeminiResponse(const String& data) {
  EthernetClient ethernetClient;
  HttpClient http(ethernetClient, "api.google.com", 80);  // Specify the server name and port (default: 80 or 443 for HTTPS)

  http.beginRequest();
  http.post("/gemini");  // Specify the endpoint path
  http.sendHeader("Authorization", "Bearer YOUR_GEMINI_API_KEY");
  http.sendHeader("Content-Type", "application/json");

  StaticJsonDocument<1024> doc;
  doc["data"] = data;
  String requestBody;
  serializeJson(doc, requestBody);
  http.sendHeader("Content-Length", requestBody.length());
  http.beginBody();
  http.print(requestBody);
  http.endRequest();

  String response = "";
  int statusCode = http.responseStatusCode();
  if (statusCode == 200) {
    response = http.responseBody();
  } else {
    response = "Error: " + String(statusCode);
  }
  return response;
}


void sendHumidityData(EthernetClient& client) {
  String response = "[";
  for (int i = 0; i < numSensors; i++) {
    float humidity = dht.readHumidity() + calibrationOffset[i];  // Apply calibration offset
    checkHumidity(i, humidity);

    // Log to SD card
    logToSD("Humidity for sensor " + String(i) + ": " + String(humidity));

    response += "{\"humidity\": " + String(humidity) + ", \"recommendedHumidity\": " + String(recommendedHumidity[i]) + "}";
    if (i < numSensors - 1) response += ",";
  }
  response += "]";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println();
  client.print(response);
}

int getPlantNumber(const String& request) {
  int startIndex = request.indexOf("plant=") + 6;  // Get index after 'plant='
  return request.substring(startIndex).toInt();
}

float getCalibrationValue(const String& request) {
  int startIndex = request.indexOf("offset=") + 7;  // Get index after 'offset='
  return request.substring(startIndex).toFloat();
}

void calibrateSensor(int plantNum, float offset) {
  calibrationOffset[plantNum] = offset;  // Store the calibration offset
  saveCalibration();                     // Save calibration to SD card
  logToSD("Calibrated sensor " + String(plantNum) + " with offset " + String(offset));
}

void waterPlant(int plantNum) {
  Serial.println("Watering plant " + String(plantNum));
  logToSD("Watering plant " + String(plantNum));
  // Add your watering mechanism here
}

void toggleFan(int plantNum) {
  Serial.println("Toggling fan for plant " + String(plantNum));
  logToSD("Toggling fan for plant " + String(plantNum));
  // Add your fan control mechanism here
}

void checkHumidity(int index, float humidity) {
  if (humidity < (recommendedHumidity[index] - 10) || humidity > (recommendedHumidity[index] + 10)) {
    sendEmailAlert(index, humidity);
  }
}

void sendEmailAlert(int plantIndex, float currentHumidity) {
  EthernetClient ethClient;
  SmtpClient smtp(&ethClient, smtpServer, 587);

  Mail mail;
  mail.from("Plant Monitor <your-email@example.com>");
  mail.replyTo("noreply@example.com");
  mail.to("Owner <owner-email@example.com>");
  mail.subject("Humidity Alert");

  String body = "Plant " + String(plantIndex + 1) + " is out of range. Current: " + String(currentHumidity) + "%";
  char bodyArray[body.length() + 1];               // Create a mutable char array
  body.toCharArray(bodyArray, sizeof(bodyArray));  // Copy the string
  mail.body(bodyArray);                            // Pass the mutable char array


  if (!smtp.send(&mail)) {
    Serial.println("Failed to send email.");
    logToSD("Failed to send email alert for plant " + String(plantIndex + 1));
  } else {
    logToSD("Email alert sent for plant " + String(plantIndex + 1) + " with humidity " + String(currentHumidity));
  }
}


void loadConfig() {
  if (SD.exists("config.json")) {
    File configFile = SD.open("config.json");
    if (configFile) {
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, configFile);
      if (!error) {
        numSensors = doc.size();
        for (int i = 0; i < numSensors; i++) {
          recommendedHumidity[i] = doc[i]["humidity"];
        }
      } else {
        Serial.println("Failed to parse config file");
        logToSD("Failed to parse config file");
      }
      configFile.close();
    }
  }
}

void loadCalibration() {
  if (SD.exists("calibration.json")) {
    File configFile = SD.open("calibration.json");
    if (configFile) {
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, configFile);
      if (!error) {
        for (int i = 0; i < MAX_SENSORS; i++) {
          calibrationOffset[i] = doc[i]["offset"];
        }
      }
      configFile.close();
    }
  }
}

void saveCalibration() {
  File configFile = SD.open("calibration.json", FILE_WRITE);
  if (configFile) {
    StaticJsonDocument<1024> doc;
    for (int i = 0; i < MAX_SENSORS; i++) {
      doc[i]["offset"] = calibrationOffset[i];
    }
    serializeJson(doc, configFile);
    configFile.close();
  }
}

void sendResponse(EthernetClient& client, const String& message) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println();
  client.print(message);
}

void logToSD(const String& message) {
  File logFile = SD.open("log.txt", FILE_WRITE);
  if (logFile) {
    logFile.println(String(millis()) + ": " + message);
    logFile.close();
  }
}