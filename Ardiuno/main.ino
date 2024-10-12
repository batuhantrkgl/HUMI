#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>

// Ethernet ayarları
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 100);

// DHT sensör ayarları
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Motor ve Fan pinleri
#define WATER_PUMP_PIN 3
#define FAN_PIN 4

EthernetServer server(80);

void setup() {
    // Ethernet'i başlat
    Ethernet.begin(mac, ip);
    server.begin();
    dht.begin();

    // Motor ve fan pinlerini çıkış olarak ayarla
    pinMode(WATER_PUMP_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(WATER_PUMP_PIN, LOW); // Başlangıçta kapalı
    digitalWrite(FAN_PIN, LOW); // Başlangıçta kapalı
}

void loop() {
    EthernetClient client = server.available();
    if (client) {
        String currentLine = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    if (currentLine.startsWith("GET /water")) {
                        // 50 ml su ekle
                        digitalWrite(WATER_PUMP_PIN, HIGH);
                        delay(5000); // Motoru 5 saniye çalıştır (50 ml için)
                        digitalWrite(WATER_PUMP_PIN, LOW);
                        client.println("HTTP/1.1 200 OK");
                        client.println();
                        client.print("Watering complete.");
                    } else if (currentLine.startsWith("GET /fan")) {
                        // Fanı aç ve 10 saniye sonra kapat
                        digitalWrite(FAN_PIN, HIGH); // Fanı aç
                        client.println("HTTP/1.1 200 OK");
                        client.println();
                        client.print("Fan turned on.");
                        delay(10000); // 10 saniye bekle
                        digitalWrite(FAN_PIN, LOW); // Fanı kapat
                    } else if (currentLine.startsWith("GET /humidity")) {
                        // Nem değerini al
                        float humidity = dht.readHumidity(); // Nem verisini al
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-Type: text/plain");
                        client.println();
                        client.print(humidity); // Nem değerini gönder
                    }
                    currentLine = "";
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        client.stop();
    }
}
