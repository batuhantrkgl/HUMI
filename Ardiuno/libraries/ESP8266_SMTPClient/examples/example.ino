#include <ESP8266WiFi.h>
#include "SMTPClient.h"

const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

SMTPClient smtp;

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(1000);

    smtp.begin("smtp.gmail.com", 465); // For SSL
    smtp.setCredentials("your_email@gmail.com", "your_password");

    if (smtp.sendMail("your_email@gmail.com", "recipient_email@gmail.com", "Test Subject", "This is a test email.")) {
        Serial.println("Email sent!");
    } else {
        Serial.println("Failed to send email.");
    }
}

void loop() {}
