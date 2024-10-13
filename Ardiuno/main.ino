#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <SD.h>
#include <SMTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

#define MAX_SENSORS 20
#define DHTPIN 2 // DHT sensor connected to digital pin 2
#define DHTTYPE DHT11 // DHT 11

DHT dht(DHTPIN, DHTTYPE);
EthernetServer server(80);
const int chipSelect = 4; // SD card module connected to pin 4
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Your MAC address
IPAddress ip(192, 168, 1, 100); // Your IP address

// SMTP settings (consider securing this)
const char* smtpServer = "smtp.your-email.com";
const char* emailUser = "your-email@example.com";
const char* emailPassword = "your-password";
const char* recipientEmail = "owner-email@example.com";

float recommendedHumidity[MAX_SENSORS] = {60.0, 60.0, 60.0}; // Recommended humidity for each plant
int numSensors = 0;
float calibrationOffset[MAX_SENSORS] = {0.0}; // Array for calibration offsets

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
    }
}

void sendHumidityData(EthernetClient& client) {
    String response = "[";
    for (int i = 0; i < numSensors; i++) {
        float humidity = dht.readHumidity() + calibrationOffset[i]; // Apply calibration offset
        checkHumidity(i, humidity);

        // Log to SD card
        File logFile = SD.open("log.txt", FILE_WRITE);
        if (logFile) {
            logFile.println(String(millis()) + "," + String(humidity));
            logFile.close();
        }

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
    int startIndex = request.indexOf("plant=") + 6; // Get index after 'plant='
    return request.substring(startIndex).toInt();
}

float getCalibrationValue(const String& request) {
    int startIndex = request.indexOf("offset=") + 7; // Get index after 'offset='
    return request.substring(startIndex).toFloat();
}

void calibrateSensor(int plantNum, float offset) {
    calibrationOffset[plantNum] = offset; // Store the calibration offset
    saveCalibration(); // Save calibration to SD card
}

void waterPlant(int plantNum) {
    Serial.println("Watering plant " + String(plantNum));
    // Add your watering mechanism here
}

void toggleFan(int plantNum) {
    Serial.println("Toggling fan for plant " + String(plantNum));
    // Add your fan control mechanism here
}

void checkHumidity(int index, float humidity) {
    if (humidity < (recommendedHumidity[index] - 10) || humidity > (recommendedHumidity[index] + 10)) {
        sendEmailAlert(index, humidity);
    }
}

void sendEmailAlert(int plantIndex, float currentHumidity) {
    SMTPClient smtp(smtpServer, 587, emailUser, emailPassword);
    smtp.setRecipient(recipientEmail);
    smtp.setSubject("Humidity Alert");
    smtp.setBody("Plant " + String(plantIndex + 1) + " is out of range. Current: " + String(currentHumidity) + "%");
    if (!smtp.send()) {
        Serial.println("Failed to send email.");
    }
}

void loadConfig() {
    // Load recommended humidity from SD or set defaults
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
