#include <ESP8266WiFi.h>
#include <DHT.h>
#include <SD.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <HttpClient.h>
#include <ESP8266HTTPClient.h>  // Add this for MailSend API requests
#include <WiFiClientSecure.h>  // Add this for HTTPS support

#define MAX_SENSORS 20
#define DHTPIN 3       // DHT sensor connected to digital pin 2
#define DHTTYPE DHT11  // DHT 11

DHT dht(DHTPIN, DHTTYPE);
WiFiServer server(80);
const int chipSelect = 4;                             // SD card module connected to pin 4

// Wi-Fi settings
const char* ssid = "example";       // Replace with your Wi-Fi SSID
const char* password = ""; // Replace with your Wi-Fi password

// MailSend API settings
const char* MAILERSEND_API_KEY = "API-KEY";
const char* MAILERSEND_ENDPOINT = "https://api.mailersend.com/v1/email";
const char* senderEmail = "something@something";  // Replace with your verified domain
const char* recipientEmail = "something@gmail.com";

float recommendedHumidity[MAX_SENSORS] = { 60.0, 60.0, 60.0 };  // Recommended humidity for each plant
int numSensors = 0;
float calibrationOffset[MAX_SENSORS] = { 0.0 };  // Array for calibration offsets

// Add global variable for SD card status
bool sdCardAvailable = false;

// Add global variable for DHT sensor status
bool dhtSensorAvailable = false;

void setup() {
  Serial.begin(115200);
  serialLog("System starting up...");
  
  // Check DHT sensor
  float testReading = dht.readHumidity();
  if (isnan(testReading)) {
    dhtSensorAvailable = false;
    serialLog("WARNING: DHT sensor not found or not working");
  } else {
    dhtSensorAvailable = true;
    serialLog("DHT sensor initialized successfully");
  }
  
  serialLog("Attempting to connect to WiFi SSID: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  serialLog("WiFi connected. IP: " + WiFi.localIP().toString());

  server.begin();
  serialLog("HTTP server started");

  // Try to initialize SD card
  if (SD.begin(chipSelect)) {
    sdCardAvailable = true;
    serialLog("SD Card initialized successfully");
  } else {
    sdCardAvailable = false;
    serialLog("WARNING: SD Card not available, using Serial logging only");
  }

  loadConfig();
  loadCalibration();
  serialLog("System initialization complete");
  
  sendBootEmail();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
}

void handleClient(WiFiClient& client) {
  serialLog("New client connection from: " + client.remoteIP().toString());
  unsigned long timeout = millis();
  String currentLine = "";
  
  while (client.connected()) {
    // Add timeout check
    if (millis() - timeout > 5000) {  // 5 second timeout
      client.stop();
      return;
    }
    
    if (client.available()) {
      char c = client.read();
      if (c == '\n') {
        if (currentLine.length() == 0) {
          break;  // End of headers
        } else {
          processRequest(currentLine, client);
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }
  
  // Ensure connection is closed
  delay(1);
  client.stop();
  serialLog("Client connection closed");
}

void processRequest(String& requestLine, WiFiClient& client) {
  serialLog("Received request: " + requestLine);
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
  
  // Limit response size if needed
  if (geminiResponse.length() > 2048) {
    geminiResponse = geminiResponse.substring(0, 2048);
  }

  // Send proper HTTP headers with Content-Length
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(geminiResponse.length());
  client.println();
  
  // Send response body
  client.print(geminiResponse);
  client.flush();
}

String readSDCardData() {
  if (!sdCardAvailable || !SD.exists("data.txt")) {
    return "Please provide me with plant care tips for indoor plants with focus on humidity requirements.";
  }
  
  String data = "";
  File dataFile = SD.open("data.txt");
  if (dataFile) {
    while (dataFile.available()) {
      data += char(dataFile.read());
    }
    dataFile.close();
  }
  return data.length() > 0 ? data : "Please provide me with plant care tips for indoor plants.";
}

String getGeminiResponse(const String& data) {
  serialLog("Requesting Gemini API response for data length: " + String(data.length()));
  
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure();
  wifiClient.setTimeout(20000);
  
  const char* host = "generativelanguage.googleapis.com";
  const int httpsPort = 443;
  
  serialLog("Connecting to Gemini API...");
  if (!wifiClient.connect(host, httpsPort)) {
    serialLog("ERROR: Connection to Gemini API failed");
    return "Error: Unable to connect to Gemini API";
  }
  
  String endpoint = "/v1beta/models/gemini-1.5-flash:generateContent";
  String apiKey = "API-KEY";
  
  StaticJsonDocument<1024> doc;
  JsonArray contents = doc.createNestedArray("contents");
  JsonObject content = contents.createNestedObject();
  JsonArray parts = content.createNestedArray("parts");
  JsonObject part = parts.createNestedObject();
  part["text"] = data;
  
  String requestBody;
  serializeJson(doc, requestBody);
  
  // Construct HTTP request with explicit content length
  String request = String("POST ") + endpoint + "?key=" + apiKey + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" +
                  "Content-Type: application/json\r\n" +
                  "Accept: application/json\r\n" +
                  "Content-Length: " + String(requestBody.length()) + "\r\n" +
                  "Connection: close\r\n\r\n" +
                  requestBody;

  serialLog("Sending request to Gemini API...");
  wifiClient.print(request);
  
  // Wait for response with timeout
  unsigned long timeout = millis();
  while (!wifiClient.available()) {
    if (millis() - timeout > 20000) {
      serialLog("ERROR: Gemini API request timeout");
      wifiClient.stop();
      return "Error: API request timed out";
    }
    delay(100);
  }
  
  String responseData = "";
  String line = "";
  bool isBody = false;
  int contentLength = -1;
  bool isChunked = false;

  // Read and process headers
  while (wifiClient.connected()) {
    line = wifiClient.readStringUntil('\n');
    line.trim();
    
    if (line.startsWith("Content-Length: ")) {
      contentLength = line.substring(16).toInt();
    }
    if (line.indexOf("Transfer-Encoding: chunked") >= 0) {
      isChunked = true;
    }
    
    // Empty line marks end of headers
    if (line.length() == 0) {
      serialLog("End of headers found");
      break;
    }
  }

  if (isChunked) {
    serialLog("Reading chunked response");
    while (wifiClient.connected()) {
      // Read chunk size
      String chunkSizeHex = wifiClient.readStringUntil('\r');
      wifiClient.read(); // Skip \n
      
      // Convert hex string to integer
      int chunkSize = (int)strtol(chunkSizeHex.c_str(), NULL, 16);
      serialLog("Chunk size: " + String(chunkSize) + " bytes");
      
      if (chunkSize == 0) break;

      // Read chunk data
      for (int i = 0; i < chunkSize && wifiClient.available(); i++) {
        responseData += (char)wifiClient.read();
      }
      
      wifiClient.read(); // Skip \r
      wifiClient.read(); // Skip \n
    }
  } else if (contentLength > 0) {
    serialLog("Reading " + String(contentLength) + " bytes");
    while (responseData.length() < contentLength && wifiClient.available()) {
      responseData += (char)wifiClient.read();
    }
  }

  wifiClient.stop();
  
  // Clean up response
  responseData.trim();
  serialLog("Response length: " + String(responseData.length()));

  if (responseData.length() == 0) {
    serialLog("ERROR: Empty response");
    return "Error: No response from API";
  }

  // Find JSON content
  int jsonStart = responseData.indexOf('{');
  int jsonEnd = responseData.lastIndexOf('}');
  
  if (jsonStart >= 0 && jsonEnd > jsonStart) {
    String jsonStr = responseData.substring(jsonStart, jsonEnd + 1);
    serialLog("Found JSON data, length: " + String(jsonStr.length()));
    
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (!error) {
      if (doc.containsKey("candidates") && 
          doc["candidates"][0].containsKey("content") &&
          doc["candidates"][0]["content"].containsKey("parts") &&
          doc["candidates"][0]["content"]["parts"][0].containsKey("text")) {
        
        String result = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
        serialLog("Successfully extracted text from JSON");
        return result;
      }
      serialLog("ERROR: Unexpected JSON structure");
    } else {
      serialLog("ERROR: JSON parse failed - " + String(error.c_str()));
    }
  } else {
    serialLog("ERROR: No JSON found in response");
  }
  
  return "Error: Unable to process API response";
}

void sendHumidityData(WiFiClient& client) {
  serialLog("Processing humidity data request");
  String response = "[";
  for (int i = 0; i < numSensors; i++) {
    float rawHumidity = -1;
    float adjustedHumidity = -1;
    
    if (dhtSensorAvailable) {
      rawHumidity = dht.readHumidity();
      if (isnan(rawHumidity)) {
        serialLog("ERROR: Failed to read from DHT sensor");
        rawHumidity = -1;
      } else {
        adjustedHumidity = rawHumidity + calibrationOffset[i];
        serialLog("Sensor " + String(i) + " - Raw: " + String(rawHumidity) + "%, Adjusted: " + String(adjustedHumidity) + "%");
      }
    } else {
      serialLog("WARNING: DHT sensor not available");
    }

    response += "{\"humidity\": " + String(adjustedHumidity) + 
                ", \"recommendedHumidity\": " + String(recommendedHumidity[i]) + 
                ", \"sensorStatus\": \"" + (dhtSensorAvailable ? "connected" : "disconnected") + "\"}";
    if (i < numSensors - 1) response += ",";
  }
  response += "]";

  serialLog("Sending humidity data response");
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
  serialLog("Starting calibration for plant " + String(plantNum) + " with offset " + String(offset));
  calibrationOffset[plantNum] = offset;  // Store the calibration offset
  saveCalibration();                     // Save calibration to SD card
  serialLog("Calibration completed for plant " + String(plantNum));
}

void waterPlant(int plantNum) {
  serialLog("Starting watering sequence for plant " + String(plantNum));
  Serial.println("Watering plant " + String(plantNum));
  serialLog("Watering plant " + String(plantNum));
  // Add your watering mechanism here
  serialLog("Completed watering for plant " + String(plantNum));
}

void toggleFan(int plantNum) {
  serialLog("Toggling fan state for plant " + String(plantNum));
  Serial.println("Toggling fan for plant " + String(plantNum));
  serialLog("Toggling fan for plant " + String(plantNum));
  // Add your fan control mechanism here
  serialLog("Fan state changed for plant " + String(plantNum));
}

void checkHumidity(int index, float humidity) {
  if (!dhtSensorAvailable) {
    serialLog("WARNING: Skipping humidity check - sensor not available");
    return;
  }
  
  serialLog("Checking humidity for plant " + String(index) + ": " + String(humidity) + "%");
  if (humidity < (recommendedHumidity[index] - 10) || humidity > (recommendedHumidity[index] + 10)) {
    serialLog("WARNING: Humidity out of range for plant " + String(index));
    sendEmailAlert(index, humidity);
  }
}

void sendBootEmail() {
  serialLog("Sending system boot notification email");
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  if (http.begin(client, MAILERSEND_ENDPOINT)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Requested-With", "XMLHttpRequest");
    http.addHeader("Authorization", "Bearer " + String(MAILERSEND_API_KEY));

    StaticJsonDocument<1024> doc;
    
    JsonObject from = doc.createNestedObject("from");
    from["email"] = senderEmail;
    from["name"] = "Plant Monitor System";
    
    JsonArray to = doc.createNestedArray("to");
    JsonObject recipient = to.createNestedObject();
    recipient["email"] = recipientEmail;
    recipient["name"] = "Plant Monitor User";

    doc["subject"] = "Plant Monitor System Boot Complete";
    
    // Simplified content structure - using only html as required by API
    doc["html"] = "<h1>Plant Monitor System Boot Complete</h1><p>The plant monitoring system has completed initialization and is now operational.</p>";

    String jsonPayload;
    serializeJson(doc, jsonPayload);
    
    serialLog("Debug - JSON Payload: " + jsonPayload);
    int httpCode = http.POST(jsonPayload);

    // Update status code check to include 202
    if (httpCode == 200 || httpCode == 202) {
      serialLog("Boot notification email accepted for delivery (Status: " + String(httpCode) + ")");
    } else {
      String response = http.getString();
      serialLog("ERROR: Failed to send boot notification. HTTP code: " + String(httpCode));
      if (response.length() > 0) {
        serialLog("Error response: " + response);
      }
    }

    http.end();
  } else {
    serialLog("ERROR: Unable to connect to MailerSend API");
  }
}

void sendEmailAlert(int plantIndex, float currentHumidity) {
  serialLog("Preparing to send email alert for plant " + String(plantIndex));
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  if (http.begin(client, MAILERSEND_ENDPOINT)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Requested-With", "XMLHttpRequest");
    http.addHeader("Authorization", "Bearer " + String(MAILERSEND_API_KEY));

    StaticJsonDocument<1024> doc;
    
    JsonObject from = doc.createNestedObject("from");
    from["email"] = senderEmail;
    from["name"] = "Plant Monitor System";
    
    JsonArray to = doc.createNestedArray("to");
    JsonObject recipient = to.createNestedObject();
    recipient["email"] = recipientEmail;
    recipient["name"] = "Plant Monitor User";

    String subject = "Plant " + String(plantIndex + 1) + " Humidity Alert";
    String htmlMessage = "<h1>Plant " + String(plantIndex + 1) + " Humidity Alert</h1>"
                        "<p>Plant " + String(plantIndex + 1) + " humidity is out of range.<br>"
                        "Current: " + String(currentHumidity) + "%</p>";

    doc["subject"] = subject;
    doc["html"] = htmlMessage;  // Only using html content

    String jsonPayload;
    serializeJson(doc, jsonPayload);
    
    serialLog("Debug - JSON Payload: " + jsonPayload);
    int httpCode = http.POST(jsonPayload);

    // Update status code check to include 202
    if (httpCode == 200 || httpCode == 202) {
      serialLog("Email alert accepted for delivery (Status: " + String(httpCode) + ")");
    } else {
      String response = http.getString();
      serialLog("ERROR: Failed to send email alert. HTTP code: " + String(httpCode));
      if (response.length() > 0) {
        serialLog("Error response: " + response);
      }
    }

    http.end();
  } else {
    serialLog("ERROR: Unable to connect to MailerSend API");
  }
}

void loadConfig() {
  serialLog("Loading configuration");
  if (sdCardAvailable && SD.exists("config.json")) {
    File configFile = SD.open("config.json");
    if (configFile) {
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, configFile);
      if (!error) {
        numSensors = doc.size();
        for (int i = 0; i < numSensors; i++) {
          recommendedHumidity[i] = doc[i]["humidity"];
        }
        serialLog("Configuration loaded successfully");
      } else {
        Serial.println("Failed to parse config file");
        serialLog("Failed to parse config file");
      }
      configFile.close();
    }
  } else {
    serialLog("Using default configuration values");
    numSensors = 1; // Set default number of sensors
    recommendedHumidity[0] = 60.0; // Set default humidity
  }
}

void loadCalibration() {
  if (sdCardAvailable && SD.exists("calibration.json")) {
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
  } else {
    serialLog("Using default calibration values");
    for (int i = 0; i < MAX_SENSORS; i++) {
      calibrationOffset[i] = 0.0;
    }
  }
}

void saveCalibration() {
  if (sdCardAvailable) {
    File configFile = SD.open("calibration.json", FILE_WRITE);
    if (configFile) {
      StaticJsonDocument<1024> doc;
      for (int i = 0; i < MAX_SENSORS; i++) {
        doc[i]["offset"] = calibrationOffset[i];
      }
      serializeJson(doc, configFile);
      configFile.close();
    }
  } else {
    serialLog("WARNING: Cannot save calibration, SD card not available");
  }
}

void sendResponse(WiFiClient& client, const String& message) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println();
  client.print(message);
}

// New function for timestamp generation
String getTimestamp() {
  return String(millis());
}

// New function for memory info
String getMemoryInfo() {
  return "RAM: " + String(ESP.getFreeHeap());
}

// New centralized logging function
void serialLog(const String& message) {
  String timestamp = getTimestamp();
  String memInfo = getMemoryInfo();
  String logEntry = timestamp + " | " + memInfo + " | " + message;
  
  // Always log to Serial
  Serial.println(logEntry);
  
  // Log to SD if available
  if (sdCardAvailable) {
    File logFile = SD.open("log.txt", FILE_WRITE);
    if (logFile) {
      logFile.println(logEntry);
      logFile.close();
    } else {
      Serial.println("WARNING: Failed to write to SD card log file");
      sdCardAvailable = false; // Disable SD logging if write fails
    }
  }
}
