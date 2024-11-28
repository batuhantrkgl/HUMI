#include <ESP8266WiFi.h>
#include <DHT.h>
#include <SD.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <HttpClient.h>
#include "SMTPClient.h"  // Include the new SMTPClient header

#define MAX_SENSORS 20
#define DHTPIN 3       // DHT sensor connected to digital pin 2
#define DHTTYPE DHT11  // DHT 11

DHT dht(DHTPIN, DHTTYPE);
WiFiServer server(80);
const int chipSelect = 4;                             // SD card module connected to pin 4

// Wi-Fi settings
const char* ssid = "fatih";       // Replace with your Wi-Fi SSID
const char* password = ""; // Replace with your Wi-Fi password

// SMTP settings
const char* smtpServer = "smtp.gmail.com";  // Replace with your SMTP server
const int smtpPort = 465;                   // Replace with your SMTP server's port
const char* smtpUsername = "your_username"; // Replace with your SMTP username
const char* smtpPassword = "your_password"; // Replace with your SMTP password

SMTPClient smtpClient;  // Create an instance of the new SMTPClient

const char* senderEmail = "sender@example.com";
const char* recipientEmail = "recipient@example.com";

float recommendedHumidity[MAX_SENSORS] = { 60.0, 60.0, 60.0 };  // Recommended humidity for each plant
int numSensors = 0;
float calibrationOffset[MAX_SENSORS] = { 0.0 };  // Array for calibration offsets

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.begin();

  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card initialization failed!");
    return;
  }

  // Initialize the SMTP client
  smtpClient.begin(smtpServer, smtpPort);
  smtpClient.setCredentials(smtpUsername, smtpPassword);

  // Load recommended humidity and calibration from SD or set defaults
  loadConfig();
  loadCalibration();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
}

void handleClient(WiFiClient& client) {
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

void processRequest(String& requestLine, WiFiClient& client) {
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

void sendPlantCareTips(WiFiClient& client) {
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
  WiFiClient wifiClient;
  HttpClient http(wifiClient, "api.google.com", 80);  // Specify the server name and port (default: 80 or 443 for HTTPS)

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

void sendHumidityData(WiFiClient& client) {
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
  String subject = "Humidity Alert";
  String body = "Plant " + String(plantIndex + 1) + " is out of range. Current: " + String(currentHumidity) + "%";

  if (smtpClient.sendMail(senderEmail, recipientEmail, subject, body)) {
    Serial.println("Email alert sent successfully!");
    logToSD("Email alert sent for plant " + String(plantIndex + 1) + " with humidity " + String(currentHumidity));
  } else {
    Serial.println("Failed to send email alert.");
    logToSD("Failed to send email alert for plant " + String(plantIndex + 1));
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

void sendResponse(WiFiClient& client, const String& message) {
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
