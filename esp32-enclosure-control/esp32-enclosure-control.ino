#include <WiFi.h>
#include <DHT.h>
#include <WiFiCredentials.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define FAN_RELAY_PIN 16  // Pin for relay control

DHT dht(DHTPIN, DHTTYPE);
// oled
#define OLED_DC    17
#define OLED_RESET 2
#define OLED_CS    -1  // or -1 if tied to GND

Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS);

WiFiServer server(80);

bool isFanRelayOn() {
  return digitalRead(FAN_RELAY_PIN) == HIGH;
}

// Terminal-style line output
int currentLine = 0;
const int lineHeight = 8; // for font size 1

String moduleName = "Creality cr 10 se";

void printToOLED(String msg) {
  display.setCursor(0, currentLine * lineHeight);
  display.println("> " + msg);
  display.display();
  currentLine++;

  if (currentLine * lineHeight >= 64) {
    delay(1000); // Optional pause before clearing
    display.clearDisplay();
    currentLine = 0;
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // oled start
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("SSD1306 allocation failed");
    for (;;); // halt
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Terminal-style OLED prints
  printToOLED("Hello from ESP32!");
  printToOLED("Connecting WiFi...");

  Serial.println("Connecting to WiFi...");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  printToOLED("WiFi connected.");
  printToOLED("IP:");
  printToOLED(WiFi.localIP().toString());

  Serial.println("\nWiFi connected. IP address:");
  Serial.println(WiFi.localIP());

  server.begin();
  pinMode(FAN_RELAY_PIN, OUTPUT);
  delay(3000);
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client connected");

    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n' && request.endsWith("\r\n\r\n")) {
          break;
        }
      }
    }

    // Parse relay command
    if (request.indexOf("GET /fan-relay?on=true") >= 0) {
      digitalWrite(FAN_RELAY_PIN, HIGH);
      Serial.println("Fan Relay ON");
    } else if (request.indexOf("GET /fan-relay?on=false") >= 0) {
      digitalWrite(FAN_RELAY_PIN, LOW);
      Serial.println("Fan Relay OFF");
    }

    // Serve JSON response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json; charset=utf-8\r\n");
    client.printf(
      "{\"temperature\": %.1f, \"humidity\": %.1f, \"fanStatus\": %s, \"moduleName\": \"%s\"}\n",
      temp, hum, isFanRelayOn() ? "true" : "false", moduleName.c_str()
    );
    client.flush();
    client.stop();
    Serial.println("Client disconnected");
  }

  // Update oled
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.printf("CR 10 SE\n");
  display.printf("Temp:%.1fC\n", temp);
  display.printf("Hum:%.1f%%\n", hum);
  display.printf("Fan: %s", isFanRelayOn() ? "ON" : "OFF");
  display.display();
  
  delay(1000);  // Refresh every second
}

